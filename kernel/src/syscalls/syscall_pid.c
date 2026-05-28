#include <stdint.h>
#include <multitasking/proc.h>
#include <multitasking/sched.h>
#include <multitasking/thread.h>
#include <syscalls/syscall_pid.h>

uint64_t sys_getpid(uint64_t a, uint64_t b, uint64_t c) {
    (void)a; (void)b; (void)c;
    return (uint64_t)get_current_pid();
}

uint64_t sys_fork(uint64_t a, uint64_t b, uint64_t c) {
    (void)a; (void)b; (void)c;
    return (uint64_t)proc_fork();
}

uint64_t sys_exec(uint64_t path, uint64_t argv, uint64_t envp) {
    return (uint64_t)proc_exec((const char *)path, (char **)argv, (char **)envp);
}

uint64_t sys_exit(uint64_t code, uint64_t b, uint64_t c) {
    (void)b; (void)c;
    proc_exit((int)code);
    __builtin_unreachable();
}

uint64_t sys_waitpid(uint64_t pid, uint64_t status, uint64_t flags) {
    return (uint64_t)proc_waitpid((int32_t)pid, (int *)status, (int)flags);
}