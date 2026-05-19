#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <mm/types.h>
#include <multitasking/thread.h>

struct pcb {
    uint16_t  pid;
    paddr     cr3;
    bool      user;
    struct tcb *threads[MAX_THREADS];
};

struct pcb *proc_create(void *entry, bool user);
struct pcb *add_thread(struct pcb *proc, void *entry);
void        proc_destroy(struct pcb *proc);