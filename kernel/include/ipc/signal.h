#pragma once

#include <multitasking/proc.h>

// Copied from https://github.com/openbsd/src/blob/master/sys/sys/signal.h
#define	SIGHUP	1	/* hangup */
#define	SIGINT	2	/* interrupt */
#define	SIGQUIT	3	/* quit */
#define	SIGILL	4	/* illegal instruction (not reset when caught) */
#define	SIGTRAP	5	/* trace trap (not reset when caught) */
#define	SIGABRT	6	/* abort() */
#define	SIGIOT	SIGABRT	/* compatibility */
#define	SIGEMT	7	/* EMT instruction */
#define	SIGFPE	8	/* floating point exception */
#define	SIGKILL	9	/* kill (cannot be caught or ignored) */
// End copy

void signal_deliver(struct pcb *proc);
void default_sig_handler(int sig);