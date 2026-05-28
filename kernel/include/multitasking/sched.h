#pragma once

#include <multitasking/thread.h>

typedef struct ready_queue {
    struct tcb *threads[MAX_THREADS];
    int front;
    int rear;
    int count;
} threads_t;

extern struct tcb *current_thread;
extern threads_t *tq;
 
void enable_sched();
void disable_sched();
void scheduler_init();
bool enqueue(struct tcb *thread);
struct tcb *dequeue();
void sched_yield();
void sched_sleep(struct tcb *t);
void sched_wake(struct tcb *t);
uintptr_t schedule(uintptr_t rsp);
int32_t get_current_pid();
struct tcb *get_current_thread();