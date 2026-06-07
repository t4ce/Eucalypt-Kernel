#include <multitasking/sched.h>
#include <multitasking/proc.h>
#include <logging/printk.h>
#include <ipc/signal.h>

void signal_deliver(struct pcb *proc) {
    if (!proc->signal_pending) {
        return;
    }

    for (int i = 0; i < NSIG; i++) {
        if (!(proc->signal_pending & (1 << i))) {
            continue;
        }

        proc->signal_pending &= ~(1 << i);
        proc->signal_handler[i](i);
    }
}

void default_sig_handler(int sig) {
    switch (sig) {
        default:
            log_debug("Signal recieved: %d\n", sig);
    }
}