#pragma once
#include <stdint.h>
#include <stddef.h>

#define AT_NULL          0
#define AT_IGNORE        1
#define AT_EXECFD        2
#define AT_PHDR          3
#define AT_PHENT         4
#define AT_PHNUM         5
#define AT_PAGESZ        6
#define AT_BASE          7
#define AT_FLAGS         8
#define AT_ENTRY         9
#define AT_NOTELF        10
#define AT_UID           11
#define AT_EUID          12
#define AT_GID           13
#define AT_EGID          14
#define AT_PLATFORM      15
#define AT_HWCAP         16
#define AT_CLKTCK        17
#define AT_SECURE        23
#define AT_BASE_PLATFORM 24
#define AT_RANDOM        25
#define AT_HWCAP2        26
#define AT_EXECFN        31
#define AT_SYSINFO_EHDR  33

#define HWCAP_X86_FPU  (1 << 0)
#define HWCAP_X86_SSE  (1 << 3)
#define HWCAP_X86_SSE2 (1 << 4)

typedef struct {
    uint64_t a_type;
    uint64_t a_val;
} auxv64_t;

typedef struct {
    uint64_t phdr_vaddr;
    uint64_t phent;
    uint64_t phnum;
    uint64_t entry;
    uint64_t interp_base;
    void    *vdso_base;
    char    *execfn;
} elf_load_info_t;

void get_random_bytes(void *buf, size_t len);

void *build_user_stack(uint64_t *pml4,
                       uint64_t  stack_top,
                       uint64_t  stack_bottom,
                       char    **argv,
                       char    **envp,
                       const elf_load_info_t *info);