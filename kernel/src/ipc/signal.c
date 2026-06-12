#include <multitasking/sched.h>
#include <multitasking/proc.h>
#include <logging/printk.h>
#include <ipc/signal.h>

static const sig_default_action_t default_actions[NSIG] = {
    [SIGHUP]  = SIG_ACTION_TERMINATE,
    [SIGINT]  = SIG_ACTION_TERMINATE,
    [SIGQUIT] = SIG_ACTION_CORE,
    [SIGILL]  = SIG_ACTION_CORE,
    [SIGTRAP] = SIG_ACTION_CORE,
    [SIGABRT] = SIG_ACTION_CORE,
    [SIGEMT]  = SIG_ACTION_CORE,
    [SIGFPE]  = SIG_ACTION_CORE,
    [SIGKILL] = SIG_ACTION_TERMINATE,
    [SIGBUS]  = SIG_ACTION_CORE,
    [SIGSEGV] = SIG_ACTION_CORE,
    [SIGSYS]  = SIG_ACTION_CORE,
    [SIGPIPE] = SIG_ACTION_TERMINATE,
    [SIGALRM] = SIG_ACTION_TERMINATE,
    [SIGTERM] = SIG_ACTION_TERMINATE,
    [SIGURG]  = SIG_ACTION_IGNORE,
    [SIGSTOP] = SIG_ACTION_STOP,
    [SIGTSTP] = SIG_ACTION_STOP,
    [SIGCONT] = SIG_ACTION_CONTINUE,
    [SIGCHLD] = SIG_ACTION_IGNORE,
    [SIGTTIN] = SIG_ACTION_STOP,
    [SIGTTOU] = SIG_ACTION_STOP,
};



void default_sig_handler(int sig) {
    switch (default_actions[sig]) {
        case SIG_ACTION_TERMINATE:
            log_debug("Signal %d: terminating\n", sig);
            proc_exit(128 + sig);
            break;
        case SIG_ACTION_CORE:
            log_debug("Signal %d: core dump + terminating\n", sig);
            proc_exit(128 + sig);
            break;
        case SIG_ACTION_STOP:
            log_debug("Signal %d: stopping\n", sig);
            sched_sleep(get_current_thread());
            break;
        case SIG_ACTION_CONTINUE:
            log_debug("Signal %d: continuing\n", sig);
            sched_wake(get_current_thread());
            break;
        case SIG_ACTION_IGNORE:
            break;
    }
}