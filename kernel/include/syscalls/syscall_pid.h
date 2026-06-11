#pragma once
#include <stdint.h>

uint64_t sys_getpid(uint64_t a, uint64_t b, uint64_t c, uint64_t d, uint64_t e, uint64_t f);
uint64_t sys_getppid(uint64_t a, uint64_t b, uint64_t c, uint64_t d, uint64_t e, uint64_t f);
uint64_t sys_fork(uint64_t a, uint64_t b, uint64_t c, uint64_t d, uint64_t e, uint64_t f);
uint64_t sys_exec(uint64_t path, uint64_t argv, uint64_t envp, uint64_t d, uint64_t e, uint64_t f);
uint64_t sys_exit(uint64_t code, uint64_t b, uint64_t c, uint64_t d, uint64_t e, uint64_t f);
uint64_t sys_exit_group(uint64_t code, uint64_t b, uint64_t c, uint64_t d, uint64_t e, uint64_t f);
uint64_t sys_waitpid(uint64_t pid, uint64_t status, uint64_t flags, uint64_t d, uint64_t e, uint64_t f);
uint64_t sys_kill(uint64_t pid, uint64_t sig, uint64_t c, uint64_t d, uint64_t e, uint64_t f);
uint64_t sys_getpgid(uint64_t pid, uint64_t b, uint64_t c, uint64_t d, uint64_t e, uint64_t f);
uint64_t sys_setpgid(uint64_t pid, uint64_t pgid, uint64_t c, uint64_t d, uint64_t e, uint64_t f);
uint64_t sys_getsid(uint64_t pid, uint64_t b, uint64_t c, uint64_t d, uint64_t e, uint64_t f);
uint64_t sys_setsid(uint64_t a, uint64_t b, uint64_t c, uint64_t d, uint64_t e, uint64_t f);
uint64_t sys_signal(uint64_t sig, uint64_t handler, uint64_t c, uint64_t d, uint64_t e, uint64_t f);
uint64_t sys_sigdefault(uint64_t sig, uint64_t b, uint64_t c, uint64_t d, uint64_t e, uint64_t f);