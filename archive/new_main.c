#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <errno.h>

#define PASS(name) printf("[PASS] %s\n", name)
#define FAIL(name, reason) printf("[FAIL] %s: %s (errno=%d)\n", name, reason, errno)

static void test_write() {
    ssize_t n = write(1, "write test\n", 11);
    if (n == 11) PASS("write");
    else FAIL("write", "wrong byte count");
}

static void test_open_close() {
    int fd = open("/test.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) { FAIL("open", "could not create /test.txt"); return; }
    close(fd);
    PASS("open/close");
}

static void test_read() {
    int fd = open("/test.txt", O_WRONLY | O_TRUNC, 0);
    if (fd < 0) { FAIL("read/setup", "open for write failed"); return; }
    write(fd, "hello", 5);
    close(fd);

    fd = open("/test.txt", O_RDONLY, 0);
    if (fd < 0) { FAIL("read", "open for read failed"); return; }

    char buf[6] = {0};
    ssize_t n = read(fd, buf, 5);
    close(fd);

    if (n == 5 && memcmp(buf, "hello", 5) == 0) PASS("read");
    else FAIL("read", "data mismatch");
}

static void test_seek_tell() {
    int fd = open("/test.txt", O_RDONLY, 0);
    if (fd < 0) { FAIL("seek/tell", "open failed"); return; }

    off_t pos = lseek(fd, 2, SEEK_SET);
    if (pos != 2) { FAIL("seek", "SEEK_SET failed"); close(fd); return; }
    PASS("seek SEEK_SET");

    pos = lseek(fd, 1, SEEK_CUR);
    if (pos != 3) { FAIL("seek", "SEEK_CUR failed"); close(fd); return; }
    PASS("seek SEEK_CUR");

    pos = lseek(fd, 0, SEEK_END);
    if (pos < 0) { FAIL("seek", "SEEK_END failed"); close(fd); return; }
    PASS("seek SEEK_END");

    close(fd);
}

static void test_stat() {
    struct stat st;
    int r = stat("/test.txt", &st);
    if (r == 0) PASS("stat");
    else FAIL("stat", "stat failed");
}

static void test_fstat() {
    int fd = open("/test.txt", O_RDONLY, 0);
    if (fd < 0) { FAIL("fstat", "open failed"); return; }
    struct stat st;
    int r = fstat(fd, &st);
    close(fd);
    if (r == 0) PASS("fstat");
    else FAIL("fstat", "fstat failed");
}

static void test_dup() {
    int fd = open("/test.txt", O_RDONLY, 0);
    if (fd < 0) { FAIL("dup", "open failed"); return; }

    int fd2 = dup(fd);
    if (fd2 < 0) { FAIL("dup", "dup failed"); close(fd); return; }

    char buf[2] = {0};
    read(fd2, buf, 1);
    close(fd);
    close(fd2);
    PASS("dup");
}

static void test_dup2() {
    int fd = open("/test.txt", O_RDONLY, 0);
    if (fd < 0) { FAIL("dup2", "open failed"); return; }

    int fd2 = dup2(fd, 10);
    if (fd2 != 10) { FAIL("dup2", "dup2 failed"); close(fd); return; }

    close(fd);
    close(fd2);
    PASS("dup2");
}

static void test_mkdir_rmdir() {
    int r = mkdir("/testdir", 0755);
    if (r < 0) { FAIL("mkdir", "mkdir failed"); return; }
    PASS("mkdir");

    r = rmdir("/testdir");
    if (r < 0) { FAIL("rmdir", "rmdir failed"); return; }
    PASS("rmdir");
}

static void test_unlink() {
    int fd = open("/todelete.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) { FAIL("unlink/setup", "create failed"); return; }
    close(fd);

    int r = unlink("/todelete.txt");
    if (r == 0) PASS("unlink");
    else FAIL("unlink", "unlink failed");
}

static void test_readdir() {
    DIR *d = opendir("/");
    if (!d) { FAIL("readdir", "opendir failed"); return; }

    struct dirent *ent;
    int count = 0;
    while ((ent = readdir(d)) != NULL)
        count++;
    closedir(d);

    if (count > 0) PASS("readdir");
    else FAIL("readdir", "no entries returned");
}

static void test_getpid() {
    pid_t pid = getpid();
    if (pid >= 0) PASS("getpid");
    else FAIL("getpid", "returned negative pid");
}

static void test_mmap_anon() {
    void *ptr = malloc(4096);
    if (ptr) {
        memset(ptr, 0xAB, 4096);
        free(ptr);
        PASS("mmap/anon (via malloc)");
    } else {
        FAIL("mmap/anon", "malloc returned NULL");
    }
}

static void test_fork_wait() {
    pid_t pid = fork();
    if (pid < 0) {
        FAIL("fork", "fork failed");
        return;
    }
    if (pid == 0) {
        exit(42);
    }
    int status = 0;
    pid_t wpid = waitpid(pid, &status, 0);
    if (wpid == pid && status == 42) PASS("fork/waitpid");
    else FAIL("fork/waitpid", "wrong pid or exit code");
}

static void test_exec() {
    pid_t pid = fork();
    if (pid < 0) { FAIL("exec/fork", "fork failed"); return; }
    if (pid == 0) {
        char *argv[] = { "/hello", NULL };
        char *envp[] = { NULL };
        execve("/hello", argv, envp);
        exit(1);
    }
    int status = 0;
    waitpid(pid, &status, 0);
    if (status == 0) PASS("exec");
    else FAIL("exec", "/hello not found or returned non-zero (expected if binary missing)");
}

int main(void) {
    printf("=== eucalypt syscall test ===\n");

    test_write();
    test_open_close();
    test_read();
    test_seek_tell();
    test_stat();
    test_fstat();
    test_dup();
    test_dup2();
    test_mkdir_rmdir();
    test_unlink();
    test_readdir();
    test_getpid();
    test_mmap_anon();
    test_fork_wait();
    test_exec();

    printf("=== done ===\n");
    return 0;
}