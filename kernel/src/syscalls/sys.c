#include <syscalls/syscall_pid.h>
#include <stdint.h>
#include <logging/printk.h>
#include <syscalls/syscall.h>
#include <syscalls/syscall_io.h>
#include <syscalls/syscall_fs.h>
#include <syscalls/syscall_memory.h>

static const syscall_fn_t syscall_table[NR_SYSCALLS] = {
    [SYS_READ]    = sys_read,
    [SYS_WRITE]   = sys_write,
    [SYS_OPEN]    = sys_open,
    [SYS_CLOSE]   = sys_close,
    [SYS_SEEK]    = sys_seek,
    [SYS_STAT]    = sys_stat,
    [SYS_MKDIR]   = sys_mkdir,
    [SYS_RMDIR]   = sys_rmdir,
    [SYS_UNLINK]  = sys_unlink,
    [SYS_READDIR] = sys_readdir,
    [SYS_DUP]     = sys_dup,
    [SYS_DUP2]    = sys_dup2,
    [SYS_TELL]    = sys_tell,
    [SYS_FSTAT]   = sys_fstat,
    [SYS_GETPID]  = sys_getpid,
    [SYS_FORK]    = sys_fork,
    [SYS_EXEC]    = sys_exec,
    [SYS_EXIT]    = sys_exit,
    [SYS_WAITPID] = sys_waitpid,
    [SYS_MMAP]    = sys_mmap,
    [SYS_MUNMAP]  = sys_munmap,
};

uint64_t do_syscall(uint64_t syscall_num, uint64_t arg0, uint64_t arg1, uint64_t arg2) {
    if (syscall_num >= NR_SYSCALLS || !syscall_table[syscall_num]) {
        log_warn("Unknown syscall %llu\n", syscall_num);
        return (uint64_t)-1;
    }
    return syscall_table[syscall_num](arg0, arg1, arg2);
}