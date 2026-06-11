#include <stdint.h>
#include <errno.h>
#include <ipc/signal.h>
#include <multitasking/proc.h>
#include <multitasking/sched.h>
#include <multitasking/thread.h>
#include <syscalls/syscall_pid.h>

uint64_t sys_getpid(uint64_t a, uint64_t b, uint64_t c, uint64_t d, uint64_t e, uint64_t f) {
    (void)a; (void)b; (void)c; (void)d; (void)e; (void)f;
    return (uint64_t)get_current_pid();
}

uint64_t sys_getppid(uint64_t a, uint64_t b, uint64_t c, uint64_t d, uint64_t e, uint64_t f) {
    (void)a; (void)b; (void)c; (void)d; (void)e; (void)f;
    return (uint64_t)get_current_ppid();
}

uint64_t sys_fork(uint64_t a, uint64_t b, uint64_t c, uint64_t d, uint64_t e, uint64_t f) {
    (void)a; (void)b; (void)c; (void)d; (void)e; (void)f;
    return (uint64_t)proc_fork();
}

uint64_t sys_exec(uint64_t path, uint64_t argv, uint64_t envp, uint64_t d, uint64_t e, uint64_t f) {
    (void)d; (void)e; (void)f;
    if (!path) return (uint64_t)-EFAULT;
    return (uint64_t)proc_exec((const char *)path, (char **)argv, (char **)envp);
}

uint64_t sys_exit(uint64_t code, uint64_t b, uint64_t c, uint64_t d, uint64_t e, uint64_t f) {
    (void)b; (void)c; (void)d; (void)e; (void)f;
    proc_exit((int)code);
    __builtin_unreachable();
}

uint64_t sys_exit_group(uint64_t code, uint64_t b, uint64_t c, uint64_t d, uint64_t e, uint64_t f) {
    (void)b; (void)c; (void)d; (void)e; (void)f;
    proc_exit_group((int)code);
    __builtin_unreachable();
}

uint64_t sys_waitpid(uint64_t pid, uint64_t status, uint64_t flags, uint64_t d, uint64_t e, uint64_t f) {
    (void)d; (void)e; (void)f;
    return (uint64_t)proc_waitpid((int32_t)pid, (int *)status, (int)flags);
}

uint64_t sys_kill(uint64_t pid, uint64_t sig, uint64_t c, uint64_t d, uint64_t e, uint64_t f) {
    (void)c; (void)d; (void)e; (void)f;
    if ((int)sig < 0 || sig >= NSIG) return (uint64_t)-EINVAL;
    int ret = proc_signal((int32_t)pid, (int)sig);
    if (ret < 0) return (uint64_t)-ESRCH;
    return 0;
}

uint64_t sys_getpgid(uint64_t pid, uint64_t b, uint64_t c, uint64_t d, uint64_t e, uint64_t f) {
    (void)b; (void)c; (void)d; (void)e; (void)f;
    struct pcb *target = pid ? proc_get((int32_t)pid) : proc_get(get_current_pid());
    if (!target) return (uint64_t)-ESRCH;
    return (uint64_t)target->pgid;
}

uint64_t sys_setpgid(uint64_t pid, uint64_t pgid, uint64_t c, uint64_t d, uint64_t e, uint64_t f) {
    (void)c; (void)d; (void)e; (void)f;
    struct pcb *target = pid ? proc_get((int32_t)pid) : proc_get(get_current_pid());
    if (!target) return (uint64_t)-ESRCH;
    target->pgid = (int32_t)pgid;
    return 0;
}

uint64_t sys_setsid(uint64_t a, uint64_t b, uint64_t c, uint64_t d, uint64_t e, uint64_t f) {
    (void)a; (void)b; (void)c; (void)d; (void)e; (void)f;
    struct pcb *proc = proc_get(get_current_pid());
    if (!proc) return (uint64_t)-ESRCH;
    if (proc->pid == proc->pgid) return (uint64_t)-EPERM;
    proc->sid  = proc->pid;
    proc->pgid = proc->pid;
    return (uint64_t)proc->sid;
}

uint64_t sys_getsid(uint64_t pid, uint64_t b, uint64_t c, uint64_t d, uint64_t e, uint64_t f) {
    (void)b; (void)c; (void)d; (void)e; (void)f;
    struct pcb *target = pid ? proc_get((int32_t)pid) : proc_get(get_current_pid());
    if (!target) return (uint64_t)-ESRCH;
    return (uint64_t)target->sid;
}

uint64_t sys_signal(uint64_t sig, uint64_t handler, uint64_t c, uint64_t d, uint64_t e, uint64_t f) {
    (void)c; (void)d; (void)e; (void)f;
    if (sig >= NSIG || sig == SIGKILL || sig == SIGSTOP) return (uint64_t)-EINVAL;
    struct pcb *proc = proc_get(get_current_pid());
    if (!proc) return (uint64_t)-ESRCH;
    sig_handler_t old = proc->signal_handler[sig];
    proc->signal_handler[sig] = (sig_handler_t)handler;
    return (uint64_t)old;
}

uint64_t sys_sigdefault(uint64_t sig, uint64_t b, uint64_t c, uint64_t d, uint64_t e, uint64_t f) {
    (void)b; (void)c; (void)d; (void)e; (void)f;
    if (sig >= NSIG || sig == SIGKILL || sig == SIGSTOP) return (uint64_t)-EINVAL;
    struct pcb *proc = proc_get(get_current_pid());
    if (!proc) return (uint64_t)-ESRCH;
    proc->signal_handler[sig] = default_sig_handler;
    return 0;
}