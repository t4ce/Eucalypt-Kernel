#include <stdint.h>
#include <drivers/fs/vfs/vfs.h>
#include <syscalls/syscall_fs.h>

uint64_t sys_stat(uint64_t arg0, uint64_t arg1, uint64_t arg2) {
    const char *path = (const char *)arg0;
    vfs_stat_t *st   = (vfs_stat_t *)arg1;
    (void)arg2;
    return (uint64_t)vfs_stat(path, st);
}

uint64_t sys_mkdir(uint64_t arg0, uint64_t arg1, uint64_t arg2) {
    const char *path = (const char *)arg0;
    (void)arg1; (void)arg2;
    return (uint64_t)vfs_mkdir(path);
}

uint64_t sys_rmdir(uint64_t arg0, uint64_t arg1, uint64_t arg2) {
    const char *path = (const char *)arg0;
    (void)arg1; (void)arg2;
    return (uint64_t)vfs_rmdir(path);
}

uint64_t sys_unlink(uint64_t arg0, uint64_t arg1, uint64_t arg2) {
    const char *path = (const char *)arg0;
    (void)arg1; (void)arg2;
    return (uint64_t)vfs_unlink(path);
}

uint64_t sys_readdir(uint64_t arg0, uint64_t arg1, uint64_t arg2) {
    const char *path  = (const char *)arg0;
    uint32_t index    = (uint32_t)arg1;
    vfs_dirent_t *out = (vfs_dirent_t *)arg2;
    return (uint64_t)vfs_readdir(path, index, out);
}