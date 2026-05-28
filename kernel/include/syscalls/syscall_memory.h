#pragma once
#include <stdint.h>

uint64_t sys_mmap(uint64_t hint, uint64_t size, uint64_t flags);
uint64_t sys_munmap(uint64_t addr, uint64_t size, uint64_t unused);