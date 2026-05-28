#include <stdint.h>
#include <drivers/fs/vfs/vfs.h>
#include <syscalls/syscall_io.h>

uint64_t sys_read(uint64_t arg0, uint64_t arg1, uint64_t arg2) {
    int fd         = (int)arg0;
    void *buf      = (void *)arg1;
    uint32_t count = (uint32_t)arg2;
    return (uint64_t)vfs_read(fd, buf, count);
}

uint64_t sys_write(uint64_t arg0, uint64_t arg1, uint64_t arg2) {
    int fd          = (int)arg0;
    const void *buf = (const void *)arg1;
    uint32_t count  = (uint32_t)arg2;
    return (uint64_t)vfs_write(fd, buf, count);
}

uint64_t sys_open(uint64_t arg0, uint64_t arg1, uint64_t arg2) {
    const char *path = (const char *)arg0;
    uint32_t flags   = (uint32_t)arg1;
    (void)arg2;
    return (uint64_t)vfs_open(path, flags);
}

uint64_t sys_close(uint64_t arg0, uint64_t arg1, uint64_t arg2) {
    int fd = (int)arg0;
    (void)arg1; (void)arg2;
    return (uint64_t)vfs_close(fd);
}

uint64_t sys_seek(uint64_t arg0, uint64_t arg1, uint64_t arg2) {
    int fd         = (int)arg0;
    int32_t offset = (int32_t)arg1;
    int whence     = (int)arg2;
    return (uint64_t)vfs_seek(fd, offset, whence);
}

uint64_t sys_dup(uint64_t arg0, uint64_t arg1, uint64_t arg2) {
    int fd = (int)arg0;
    (void)arg1; (void)arg2;
    return (uint64_t)vfs_dup(fd);
}

uint64_t sys_dup2(uint64_t arg0, uint64_t arg1, uint64_t arg2) {
    int old_fd = (int)arg0;
    int new_fd = (int)arg1;
    (void)arg2;
    return (uint64_t)vfs_dup2(old_fd, new_fd);
}

uint64_t sys_tell(uint64_t arg0, uint64_t arg1, uint64_t arg2) {
    int fd = (int)arg0;
    (void)arg1; (void)arg2;
    return (uint64_t)vfs_tell(fd);
}

uint64_t sys_fstat(uint64_t arg0, uint64_t arg1, uint64_t arg2) {
    int fd         = (int)arg0;
    vfs_stat_t *st = (vfs_stat_t *)arg1;
    (void)arg2;
    return (uint64_t)vfs_fstat(fd, st);
}