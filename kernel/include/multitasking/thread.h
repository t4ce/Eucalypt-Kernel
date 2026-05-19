#pragma once
#include <stdint.h>
#include <mm/types.h>

#define KERNEL_STACK_SIZE 0x10000
#define MAX_THREADS       1024

typedef enum { ready, running, blocked, dead } thread_state_t;

struct tcb {
    uint16_t       tid;
    struct tcb    *parent;
    paddr          cr3;
    thread_state_t state;
    void          *stack_base;
    void          *ustack_base;
    void          *entry;
    uint64_t       rsp;
};

struct tcb  *create_thread(void *entry, paddr cr3);
struct tcb  *create_user_thread(uint64_t entry, paddr cr3);
struct tcb  *get_thread_copy(uint16_t tid);
struct tcb **get_thread(uint16_t tid);
void         thread_destroy(struct tcb *thread);
void         handle_ret(int64_t code);
uint64_t     setup_stack(uint8_t *stack_base, uint64_t stack_size, void *entry);