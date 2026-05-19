#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <mm/types.h>
#include <mm/heap.h>
#include <mm/frame.h>
#include <mm/paging.h>
#include <mm/hhdm.h>
#include <logging/printk.h>
#include <multitasking/sched.h>
#include <multitasking/thread.h>

#define KERNEL_CS 0x08
#define KERNEL_SS 0x10
#define USER_CS   0x23
#define USER_SS   0x1B

extern void    thread_trampoline();
extern uintptr_t offset;

uint16_t next_tid = 0;

struct stack_alloc {
    void *raw;
    void *aligned;
};

static struct stack_alloc alloc_aligned_stack(size_t size) {
    struct stack_alloc stack = {0};

    uintptr_t raw = (uintptr_t)kmalloc(size + 0x1000);
    if (!raw)
        return stack;

    stack.raw     = (void *)raw;
    stack.aligned = (void *)((raw + 0xFFF) & ~0xFFFULL);
    return stack;
}

static uint64_t alloc_user_stack(uint64_t *pml4) {
    const uint64_t base  = 0x70000000000;
    const uint64_t pages = 4;
    const uint64_t flags = ENTRY_FLAG_PRESENT | ENTRY_FLAG_RW |
                           ENTRY_FLAG_NX      | ENTRY_FLAG_USER;

    for (uint64_t i = 0; i < pages; i++) {
        paddr frame = frame_alloc();
        vaddr virt  = base + (i * 0x1000);
        paging_map_page(pml4, virt, frame, 0x1000, flags);
    }

    return base + (pages * 0x1000);
}

uint64_t setup_stack(uint8_t *stack_base, uint64_t stack_size, void *entry) {
    uint64_t *stack_top = (uint64_t *)(stack_base + stack_size);
    uint64_t *rsp       = stack_top;

    *--rsp = KERNEL_SS;
    *--rsp = (uint64_t)stack_top;
    *--rsp = 0x202;
    *--rsp = KERNEL_CS;
    *--rsp = (uint64_t)thread_trampoline;

    for (int i = 0; i < 15; i++)
        *--rsp = 0;

    rsp[13] = (uint64_t)entry;   /* rbx slot */

    return (uint64_t)rsp;
}

struct tcb *create_user_thread(uint64_t entry, paddr cr3) {
    uint64_t *pml4      = (uint64_t *)(offset + cr3);
    uint64_t user_stack = alloc_user_stack(pml4);

    struct stack_alloc kstack = alloc_aligned_stack(KERNEL_STACK_SIZE);
    if (!kstack.aligned)
        return NULL;

    struct tcb *tcb = kmalloc(sizeof(struct tcb));
    if (!tcb) {
        kfree(kstack.raw);
        return NULL;
    }

    uint64_t *rsp = (uint64_t *)((uint8_t *)kstack.aligned + KERNEL_STACK_SIZE);

    *--rsp = USER_SS;       /* SS            */
    *--rsp = user_stack;    /* RSP           */
    *--rsp = 0x202;         /* RFLAGS        */
    *--rsp = USER_CS;       /* CS            */
    *--rsp = entry;         /* RIP           */

    for (int i = 0; i < 15; i++)
        *--rsp = 0;

    log_debug("User thread %d cr3: %llX entry: %llX ustack: %llX",
              next_tid, cr3, entry, user_stack);

    tcb->tid         = next_tid++;
    tcb->parent      = NULL;
    tcb->cr3         = cr3;
    tcb->state       = ready;
    tcb->stack_base  = kstack.raw;
    tcb->ustack_base = NULL;
    tcb->entry       = (void *)entry;
    tcb->rsp         = (uint64_t)rsp;

    enqueue(tcb);
    return tcb;
}

struct tcb *create_thread(void *entry, paddr cr3) {
    struct stack_alloc kstack = alloc_aligned_stack(KERNEL_STACK_SIZE);
    if (!kstack.aligned)
        return NULL;

    struct tcb *tcb = kmalloc(sizeof(struct tcb));
    if (!tcb) {
        kfree(kstack.raw);
        return NULL;
    }

    log_debug("Thread %d cr3: %llX", next_tid, cr3);

    tcb->tid         = next_tid++;
    tcb->parent      = NULL;
    tcb->cr3         = cr3;
    tcb->state       = ready;
    tcb->stack_base  = kstack.raw;
    tcb->ustack_base = NULL;
    tcb->entry       = entry;
    tcb->rsp         = setup_stack(
                           (uint8_t *)kstack.aligned,
                           KERNEL_STACK_SIZE,
                           entry);

    enqueue(tcb);
    return tcb;
}

struct tcb *get_thread_copy(uint16_t tid) {
    for (int i = 0; i < tq->count; i++) {
        int idx = (tq->front + i) % MAX_THREADS;
        if (tq->threads[idx]->tid == tid)
            return tq->threads[idx];
    }
    return NULL;
}

struct tcb **get_thread(uint16_t tid) {
    for (int i = 0; i < tq->count; i++) {
        int idx = (tq->front + i) % MAX_THREADS;
        if (tq->threads[idx]->tid == tid)
            return &tq->threads[idx];
    }
    return NULL;
}

void thread_destroy(struct tcb *thread) {
    if (!thread)
        return;
    log_info("Freeing thread %d", thread->tid);
    kfree(thread->ustack_base);
    kfree(thread->stack_base);
    frame_free(thread->cr3);
    kfree(thread);
}

void handle_ret(int64_t code) {
    __asm__ volatile("cli");
    current_thread->state = dead;
    log_info("Thread %d exited with code %ld", current_thread->tid, code);
    __asm__ volatile("int $32");
    __builtin_unreachable();
}