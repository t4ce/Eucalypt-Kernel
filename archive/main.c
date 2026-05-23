#include <stdint.h>

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

#define VFS_O_RDONLY  0x0000
#define VFS_O_WRONLY  0x0001
#define VFS_O_RDWR    0x0002
#define VFS_O_CREAT   0x0040
#define VFS_O_TRUNC   0x0200

#define VFS_SEEK_SET  0
#define VFS_SEEK_CUR  1
#define VFS_SEEK_END  2

#define STDOUT 1
#define STDERR 2

#define MAX_NAME_LEN 256

typedef struct {
    char     name[MAX_NAME_LEN];
    uint32_t type;
    uint32_t size;
    uint32_t flags;
} vfs_stat_t;

typedef struct {
    char     name[MAX_NAME_LEN];
    uint32_t type;
} vfs_dirent_t;

static uint64_t syscall(uint64_t num, uint64_t arg0, uint64_t arg1, uint64_t arg2) {
    uint64_t ret;
    __asm__ volatile (
        "int $0x80\n"
        : "=a" (ret)
        : "a" (num), "D" (arg0), "S" (arg1), "d" (arg2)
        : "memory"
    );
    return ret;
}

static void write_str(int fd, const char *s) {
    uint32_t len = 0;
    while (s[len]) len++;
    syscall(SYS_WRITE, fd, (uint64_t)s, len);
}

static void write_char(int fd, char c) {
    syscall(SYS_WRITE, fd, (uint64_t)&c, 1);
}

static void write_uint(int fd, uint64_t v) {
    if (v == 0) { write_char(fd, '0'); return; }
    char buf[20];
    int i = 0;
    while (v) { buf[i++] = '0' + (v % 10); v /= 10; }
    while (i--) write_char(fd, buf[i]);
}

static void write_int(int fd, int64_t v) {
    if (v < 0) { write_char(fd, '-'); v = -v; }
    write_uint(fd, (uint64_t)v);
}

static void pass(const char *name) {
    write_str(STDOUT, "[PASS] ");
    write_str(STDOUT, name);
    write_char(STDOUT, '\n');
}

static void fail(const char *name, int64_t got) {
    write_str(STDERR, "[FAIL] ");
    write_str(STDERR, name);
    write_str(STDERR, " (got ");
    write_int(STDERR, got);
    write_str(STDERR, ")\n");
}

static void check(const char *name, int64_t result, int64_t expected) {
    if (result == expected) pass(name);
    else fail(name, result);
}

void _start(void) {
    write_str(STDOUT, "=== VFS test suite ===\n");

    write_str(STDOUT, "\n-- stdout/stderr --\n");
    write_str(STDOUT, "hello from fd 1 (stdout)\n");
    write_str(STDERR, "hello from fd 2 (stderr)\n");

    write_str(STDOUT, "\n-- open/write/close --\n");
    int64_t fd = syscall(SYS_OPEN, (uint64_t)"/C:/hello.txt",
                         VFS_O_WRONLY | VFS_O_CREAT | VFS_O_TRUNC, 0);
    check("open creat", fd >= 0 ? 0 : fd, 0);

    if (fd >= 0) {
        const char *content = "Hello, VFS!\n";
        uint32_t clen = 12;
        int64_t w = syscall(SYS_WRITE, fd, (uint64_t)content, clen);
        check("write returns byte count", w, clen);

        int64_t pos = syscall(SYS_TELL, fd, 0, 0);
        check("tell after write", pos, clen);

        int64_t c = syscall(SYS_CLOSE, fd, 0, 0);
        check("close", c, 0);
    }

    write_str(STDOUT, "\n-- open/seek/read --\n");
    fd = syscall(SYS_OPEN, (uint64_t)"/C:/hello.txt", VFS_O_RDONLY, 0);
    check("open rdonly", fd >= 0 ? 0 : fd, 0);

    if (fd >= 0) {
        char buf[64];
        for (int i = 0; i < 64; i++) buf[i] = 0;

        int64_t r = syscall(SYS_READ, fd, (uint64_t)buf, 12);
        check("read 12 bytes", r, 12);

        write_str(STDOUT, "read back: ");
        write_str(STDOUT, buf);

        int64_t pos = syscall(SYS_TELL, fd, 0, 0);
        check("tell after read", pos, 12);

        int64_t s = syscall(SYS_SEEK, fd, 0, VFS_SEEK_SET);
        check("seek to start", s, 0);

        pos = syscall(SYS_TELL, fd, 0, 0);
        check("tell at start", pos, 0);

        s = syscall(SYS_SEEK, fd, 0, VFS_SEEK_END);
        check("seek to end", s, 12);

        int64_t c = syscall(SYS_CLOSE, fd, 0, 0);
        check("close rdonly", c, 0);
    }

    write_str(STDOUT, "\n-- stat --\n");
    vfs_stat_t st;
    int64_t s = syscall(SYS_STAT, (uint64_t)"/C:/hello.txt", (uint64_t)&st, 0);
    check("stat returns 0", s, 0);
    if (s == 0) {
        check("stat size", st.size, 12);
        write_str(STDOUT, "stat name: ");
        write_str(STDOUT, st.name);
        write_char(STDOUT, '\n');
    }

    write_str(STDOUT, "\n-- fstat --\n");
    fd = syscall(SYS_OPEN, (uint64_t)"/C:/hello.txt", VFS_O_RDONLY, 0);
    if (fd >= 0) {
        vfs_stat_t fst;
        int64_t fs = syscall(SYS_FSTAT, fd, (uint64_t)&fst, 0);
        check("fstat returns 0", fs, 0);
        if (fs == 0) check("fstat size", fst.size, 12);
        syscall(SYS_CLOSE, fd, 0, 0);
    }

    write_str(STDOUT, "\n-- mkdir/readdir --\n");
    int64_t md = syscall(SYS_MKDIR, (uint64_t)"/C:/testdir", 0, 0);
    check("mkdir", md, 0);

    fd = syscall(SYS_OPEN, (uint64_t)"/C:/testdir/inner.txt",
                 VFS_O_WRONLY | VFS_O_CREAT | VFS_O_TRUNC, 0);
    check("create file in subdir", fd >= 0 ? 0 : fd, 0);
    if (fd >= 0) syscall(SYS_CLOSE, fd, 0, 0);

    vfs_dirent_t dent;
    int64_t rd = syscall(SYS_READDIR, (uint64_t)"/C:/testdir", 0, (uint64_t)&dent);
    check("readdir index 0", rd, 0);
    if (rd == 0) {
        write_str(STDOUT, "readdir[0]: ");
        write_str(STDOUT, dent.name);
        write_char(STDOUT, '\n');
    }

    write_str(STDOUT, "\n-- dup/dup2 --\n");
    fd = syscall(SYS_OPEN, (uint64_t)"/C:/hello.txt", VFS_O_RDONLY, 0);
    if (fd >= 0) {
        int64_t fd2 = syscall(SYS_DUP, fd, 0, 0);
        check("dup returns new fd", fd2 != fd ? 0 : -1, 0);
        check("dup fd is valid", fd2 >= 0 ? 0 : fd2, 0);

        int64_t fd3 = 20;
        int64_t d2  = syscall(SYS_DUP2, fd, fd3, 0);
        check("dup2 returns target fd", d2, fd3);

        syscall(SYS_CLOSE, fd,  0, 0);
        syscall(SYS_CLOSE, fd2, 0, 0);
        syscall(SYS_CLOSE, fd3, 0, 0);
    }

    write_str(STDOUT, "\n-- unlink --\n");
    int64_t ul = syscall(SYS_UNLINK, (uint64_t)"/C:/hello.txt", 0, 0);
    check("unlink", ul, 0);

    vfs_stat_t gone;
    int64_t sg = syscall(SYS_STAT, (uint64_t)"/C:/hello.txt", (uint64_t)&gone, 0);
    check("stat after unlink fails", sg, -1);

    write_str(STDOUT, "\n-- rmdir --\n");
    int64_t ul2 = syscall(SYS_UNLINK, (uint64_t)"/C:/testdir/inner.txt", 0, 0);
    check("unlink inner.txt", ul2, 0);

    int64_t rm = syscall(SYS_RMDIR, (uint64_t)"/C:/testdir", 0, 0);
    check("rmdir", rm, 0);

    write_str(STDOUT, "\n-- /dev nodes --\n");
    int64_t nfd = syscall(SYS_OPEN, (uint64_t)"/dev/null", VFS_O_WRONLY, 0);
    check("open /dev/null", nfd >= 0 ? 0 : nfd, 0);
    if (nfd >= 0) {
        int64_t nw = syscall(SYS_WRITE, nfd, (uint64_t)"discard", 7);
        check("write to null", nw, 7);
        syscall(SYS_CLOSE, nfd, 0, 0);
    }

    int64_t zfd = syscall(SYS_OPEN, (uint64_t)"/dev/zero", VFS_O_RDONLY, 0);
    check("open /dev/zero", zfd >= 0 ? 0 : zfd, 0);
    if (zfd >= 0) {
        char zbuf[8];
        for (int i = 0; i < 8; i++) zbuf[i] = 0xFF;
        int64_t zr = syscall(SYS_READ, zfd, (uint64_t)zbuf, 8);
        check("read from zero returns 8", zr, 8);
        int allzero = 1;
        for (int i = 0; i < 8; i++) if (zbuf[i] != 0) allzero = 0;
        check("zero bytes are 0x00", allzero, 1);
        syscall(SYS_CLOSE, zfd, 0, 0);
    }

    write_str(STDOUT, "\n=== done ===\n");

    for (;;) __asm__ volatile ("nop");
}