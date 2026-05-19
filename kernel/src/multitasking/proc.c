#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <mm/types.h>
#include <mm/heap.h>
#include <mm/paging.h>
#include <multitasking/thread.h>
#include <multitasking/sched.h>
#include <multitasking/proc.h>

uint16_t next_pid = 0;

struct pcb *proc_create(void *entry, bool user) {
    struct pcb *pcb = kmalloc(sizeof(struct pcb));
    if (!pcb)
        return NULL;

    pcb->pid  = next_pid++;
    pcb->cr3  = paging_create_pml4();
    pcb->user = user;

    for (int i = 0; i < MAX_THREADS; i++)
        pcb->threads[i] = NULL;

    pcb->threads[0] = user
        ? create_user_thread((uint64_t)entry, pcb->cr3)
        : create_thread(entry, pcb->cr3);

    return pcb;
}

struct pcb *add_thread(struct pcb *proc, void *entry) {
    for (int i = 0; i < MAX_THREADS; i++) {
        if (proc->threads[i] == NULL) {
            proc->threads[i] = proc->user
                ? create_user_thread((uint64_t)entry, proc->cr3)
                : create_thread(entry, proc->cr3);
            return proc;
        }
    }
    return NULL;
}

void proc_destroy(struct pcb *proc) {
    for (int i = 0; i < MAX_THREADS; i++) {
        if (proc->threads[i])
            thread_destroy(proc->threads[i]);
    }
    kfree(proc);
}