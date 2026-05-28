#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <mm/types.h>
#include <multitasking/thread.h>

#define MAX_FDS     64

typedef enum {
    PROC_RUNNING,
    PROC_SLEEPING,
    PROC_ZOMBIE,
    PROC_DEAD,
} proc_state_t;

struct pcb {
    int32_t      pid;
    int32_t      parent_pid;
    paddr        cr3;
    bool         user;
    proc_state_t state;
    int          exit_code;
    struct tcb  *threads[MAX_THREADS];
    void        *fd_table[MAX_FDS];
    uintptr_t    heap_start;
    uintptr_t    heap_end;
};

struct pcb *proc_create(void *entry, bool user);
struct pcb *proc_fork(void);
int         proc_exec(const char *path, char **argv, char **envp);
void        proc_exit(int code);
int32_t     proc_waitpid(int32_t pid, int *status, int flags);
struct pcb *add_thread(struct pcb *proc, void *entry);
void        proc_destroy(struct pcb *proc);
struct pcb *proc_get(int32_t pid);