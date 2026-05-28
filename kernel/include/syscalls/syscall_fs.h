#pragma once

#include <stdint.h>

uint64_t sys_stat(uint64_t arg0, uint64_t arg1, uint64_t arg2);
uint64_t sys_mkdir(uint64_t arg0, uint64_t arg1, uint64_t arg2);
uint64_t sys_rmdir(uint64_t arg0, uint64_t arg1, uint64_t arg2);
uint64_t sys_unlink(uint64_t arg0, uint64_t arg1, uint64_t arg2);
uint64_t sys_readdir(uint64_t arg0, uint64_t arg1, uint64_t arg2);