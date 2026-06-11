#pragma once

#include <multitasking/proc.h>

#define SIGHUP   1
#define SIGINT   2
#define SIGQUIT  3
#define SIGILL   4
#define SIGTRAP  5
#define SIGABRT  6
#define SIGIOT   SIGABRT
#define SIGEMT   7
#define SIGFPE   8
#define SIGKILL  9
#define SIGBUS   10
#define SIGSEGV  11
#define SIGSYS   12
#define SIGPIPE  13
#define SIGALRM  14
#define SIGTERM  15
#define SIGURG   16
#define SIGSTOP  17
#define SIGTSTP  18
#define SIGCONT  19
#define SIGCHLD  20
#define SIGTTIN  21
#define SIGTTOU  22
#define NSIG     32

typedef void (*sig_handler_t)(int);

typedef enum {
    SIG_ACTION_TERMINATE,
    SIG_ACTION_IGNORE,
    SIG_ACTION_CORE,
    SIG_ACTION_STOP,
    SIG_ACTION_CONTINUE,
} sig_default_action_t;

void signal_deliver(struct pcb *proc);
void default_sig_handler(int sig);