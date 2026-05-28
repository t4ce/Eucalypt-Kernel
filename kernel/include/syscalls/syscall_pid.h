#pragma once

#include <stdint.h>

uint64_t sys_getpid(uint64_t a, uint64_t b, uint64_t c);
uint64_t sys_fork(uint64_t a, uint64_t b, uint64_t c);
uint64_t sys_exec(uint64_t path, uint64_t argv, uint64_t envp);
uint64_t sys_exit(uint64_t code, uint64_t b, uint64_t c);
uint64_t sys_waitpid(uint64_t pid, uint64_t status, uint64_t flags);