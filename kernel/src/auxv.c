#include <stdint.h>
#include <mem.h>
#include <mm/hhdm.h>
#include <mm/paging.h>
#include <crypto/randomness.h>
#include <auxv.h>

#define PAGE_SIZE 0x1000ULL
#define ADDR_MASK 0x000FFFFFFFFFF000ULL

extern uintptr_t offset;

static uint8_t *hhdm_ptr(uint64_t *pml4, uint64_t uaddr) {
    uint64_t entry = paging_get_entry(pml4, uaddr & ~(PAGE_SIZE - 1));
    if (!entry) return NULL;
    paddr phys = entry & ADDR_MASK;
    return (uint8_t *)(offset + phys + (uaddr & (PAGE_SIZE - 1)));
}

static bool stack_write(uint64_t *pml4, uint64_t uaddr, const void *src, size_t len) {
    const uint8_t *s = src;
    while (len) {
        uint8_t *dst = hhdm_ptr(pml4, uaddr);
        if (!dst) return false;
        uint64_t page_end = (uaddr & ~(PAGE_SIZE - 1)) + PAGE_SIZE;
        size_t   chunk    = page_end - uaddr;
        if (chunk > len) chunk = len;
        memcpy(dst, s, chunk);
        uaddr += chunk;
        s     += chunk;
        len   -= chunk;
    }
    return true;
}

static bool stack_write_u64(uint64_t *pml4, uint64_t uaddr, uint64_t val) {
    return stack_write(pml4, uaddr, &val, 8);
}

void *build_user_stack(uint64_t *pml4,
                       uint64_t  stack_top,
                       uint64_t  stack_bottom,
                       char    **argv,
                       char    **envp,
                       const elf_load_info_t *info)
{
    uint64_t sp = stack_top;

#define PUSH_BYTES(src, len)                                \
    do {                                                    \
        sp -= (len);                                        \
        if (sp < stack_bottom) return NULL;                 \
        if (!stack_write(pml4, sp, (src), (len)))           \
            return NULL;                                    \
    } while (0)

#define PUSH_U64(val)                                       \
    do {                                                    \
        sp -= 8;                                            \
        if (sp < stack_bottom) return NULL;                 \
        if (!stack_write_u64(pml4, sp, (uint64_t)(val)))   \
            return NULL;                                    \
    } while (0)

#define PUSH_AUX(type, val) \
    do { PUSH_U64(val); PUSH_U64(type); } while (0)

    static const char platform[] = "x86_64";
    PUSH_BYTES(platform, sizeof(platform));
    uint64_t platform_addr = sp;

    const char *execfn = info->execfn ? info->execfn : "";
    PUSH_BYTES(execfn, strlen(execfn) + 1);
    uint64_t execfn_addr = sp;

    int envc = 0;
    while (envp && envp[envc]) envc++;
    uint64_t envp_addrs[envc + 1];
    for (int i = envc - 1; i >= 0; i--) {
        PUSH_BYTES(envp[i], strlen(envp[i]) + 1);
        envp_addrs[i] = sp;
    }
    envp_addrs[envc] = 0;

    int argc = 0;
    while (argv && argv[argc]) argc++;
    uint64_t argv_addrs[argc + 1];
    for (int i = argc - 1; i >= 0; i--) {
        PUSH_BYTES(argv[i], strlen(argv[i]) + 1);
        argv_addrs[i] = sp;
    }
    argv_addrs[argc] = 0;

    sp -= 16;
    if (sp < stack_bottom) return NULL;
    uint64_t rand_buf[2];
    get_random_bytes(rand_buf, 16);
    if (!stack_write(pml4, sp, rand_buf, 16)) return NULL;
    uint64_t random_addr = sp;

    sp &= ~0xFULL;

    PUSH_AUX(AT_NULL,         0);
    PUSH_AUX(AT_PLATFORM,     platform_addr);
    PUSH_AUX(AT_EXECFN,       execfn_addr);
    PUSH_AUX(AT_RANDOM,       random_addr);
    PUSH_AUX(AT_SECURE,       0);
    PUSH_AUX(AT_EGID,         0);
    PUSH_AUX(AT_GID,          0);
    PUSH_AUX(AT_EUID,         0);
    PUSH_AUX(AT_UID,          0);
    PUSH_AUX(AT_CLKTCK,       100);
    PUSH_AUX(AT_HWCAP,        HWCAP_X86_FPU | HWCAP_X86_SSE | HWCAP_X86_SSE2);
    PUSH_AUX(AT_FLAGS,        0);
    if (info->vdso_base)
        PUSH_AUX(AT_SYSINFO_EHDR, (uint64_t)info->vdso_base);
    if (info->interp_base)
        PUSH_AUX(AT_BASE,     info->interp_base);
    PUSH_AUX(AT_ENTRY,        info->entry);
    PUSH_AUX(AT_PAGESZ,       PAGE_SIZE);
    PUSH_AUX(AT_PHNUM,        info->phnum);
    PUSH_AUX(AT_PHENT,        info->phent);
    PUSH_AUX(AT_PHDR,         info->phdr_vaddr);

    PUSH_U64(0);
    for (int i = envc - 1; i >= 0; i--)
        PUSH_U64(envp_addrs[i]);

    PUSH_U64(0);
    for (int i = argc - 1; i >= 0; i--)
        PUSH_U64(argv_addrs[i]);

    PUSH_U64(argc);

#undef PUSH_AUX
#undef PUSH_BYTES
#undef PUSH_U64

    return (void *)sp;
}