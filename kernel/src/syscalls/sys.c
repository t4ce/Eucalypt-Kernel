#include <stdint.h>
#include <logging/printk.h>
#include <drivers/fs/vfs/vfs.h>

#define SYS_READ    0
#define SYS_WRITE   1
#define SYS_OPEN    2
#define SYS_CLOSE   3
#define SYS_SEEK    4
#define SYS_STAT    5
#define SYS_MKDIR   6
#define SYS_RMDIR   7
#define SYS_UNLINK  8
#define SYS_READDIR 9
#define SYS_DUP     10
#define SYS_DUP2    11
#define SYS_TELL    12
#define SYS_FSTAT   13

uint64_t do_syscall(uint64_t syscall_num, uint64_t arg0, uint64_t arg1, uint64_t arg2) {
    log_debug("Syscall called with number %d and args %llx, %llx, %llx\n",
              syscall_num, arg0, arg1, arg2);

    switch (syscall_num) {

        case SYS_WRITE: {
            int fd          = (int)arg0;
            const void *buf = (const void *)arg1;
            uint32_t count  = (uint32_t)arg2;
            return (uint64_t)vfs_write(fd, buf, count);
        }

        case SYS_READ: {
            int fd      = (int)arg0;
            void *buf   = (void *)arg1;
            uint32_t count = (uint32_t)arg2;
            return (uint64_t)vfs_read(fd, buf, count);
        }

        case SYS_OPEN: {
            const char *path = (const char *)arg0;
            uint32_t flags   = (uint32_t)arg1;
            return (uint64_t)vfs_open(path, flags);
        }

        case SYS_CLOSE: {
            int fd = (int)arg0;
            return (uint64_t)vfs_close(fd);
        }

        case SYS_SEEK: {
            int fd         = (int)arg0;
            int32_t offset = (int32_t)arg1;
            int whence     = (int)arg2;
            return (uint64_t)vfs_seek(fd, offset, whence);
        }

        case SYS_STAT: {
            const char *path = (const char *)arg0;
            vfs_stat_t *st   = (vfs_stat_t *)arg1;
            return (uint64_t)vfs_stat(path, st);
        }

        case SYS_MKDIR: {
            const char *path = (const char *)arg0;
            return (uint64_t)vfs_mkdir(path);
        }

        case SYS_RMDIR: {
            const char *path = (const char *)arg0;
            return (uint64_t)vfs_rmdir(path);
        }

        case SYS_UNLINK: {
            const char *path = (const char *)arg0;
            return (uint64_t)vfs_unlink(path);
        }

        case SYS_READDIR: {
            const char *path  = (const char *)arg0;
            uint32_t index    = (uint32_t)arg1;
            vfs_dirent_t *out = (vfs_dirent_t *)arg2;
            return (uint64_t)vfs_readdir(path, index, out);
        }

        case SYS_DUP: {
            int fd = (int)arg0;
            return (uint64_t)vfs_dup(fd);
        }

        case SYS_DUP2: {
            int old_fd = (int)arg0;
            int new_fd = (int)arg1;
            return (uint64_t)vfs_dup2(old_fd, new_fd);
        }

        case SYS_TELL: {
            int fd = (int)arg0;
            return (uint64_t)vfs_tell(fd);
        }

        case SYS_FSTAT: {
            int fd         = (int)arg0;
            vfs_stat_t *st = (vfs_stat_t *)arg1;
            return (uint64_t)vfs_fstat(fd, st);
        }

        default:
            log_warn("Unknown syscall %llu\n", syscall_num);
            return (uint64_t)-1;
    }
}