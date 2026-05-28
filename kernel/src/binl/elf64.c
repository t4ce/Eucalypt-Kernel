#include <stdint.h>
#include <mem.h>
#include <mm/hhdm.h>
#include <mm/frame.h>
#include <mm/heap.h>
#include <mm/paging.h>
#include <drivers/fs/vfs/vfs.h>
#include <binl/elf64.h>
#include <auxv.h>

uint64_t elf64_parse(int fd, paddr cr3, elf_load_info_t *info) {
    int32_t size = vfs_filesize(fd);
    if (size <= 0)
        return 0;

    void *elf = kmalloc(size);
    if (!elf)
        return 0;

    if (vfs_seek(fd, 0, VFS_SEEK_SET) < 0) {
        kfree(elf);
        return 0;
    }

    if (vfs_read(fd, elf, size) != size) {
        kfree(elf);
        return 0;
    }

    Elf64_Ehdr *eh = (Elf64_Ehdr *)elf;

    if (eh->e_ident[EI_MAG0] != ELFMAG0 ||
        eh->e_ident[EI_MAG1] != ELFMAG1 ||
        eh->e_ident[EI_MAG2] != ELFMAG2 ||
        eh->e_ident[EI_MAG3] != ELFMAG3) {
        kfree(elf);
        return 0;
    }

    if (eh->e_ident[EI_CLASS] != ELFCLASS64) {
        kfree(elf);
        return 0;
    }

    uint64_t *pml4 = (uint64_t *)(offset + cr3);
    Elf64_Phdr *ph = (Elf64_Phdr *)((uint8_t *)elf + eh->e_phoff);

    uint64_t phdr_vaddr = 0;

    for (int i = 0; i < eh->e_phnum; i++) {
        if (ph[i].p_type == PT_PHDR) {
            phdr_vaddr = ph[i].p_vaddr;
        }

        if (ph[i].p_type != PT_LOAD)
            continue;

        if (phdr_vaddr == 0 && eh->e_phoff >= ph[i].p_offset &&
            eh->e_phoff < ph[i].p_offset + ph[i].p_filesz) {
            phdr_vaddr = ph[i].p_vaddr + (eh->e_phoff - ph[i].p_offset);
        }

        uint64_t seg_vaddr   = ph[i].p_vaddr;
        uint64_t seg_offset  = ph[i].p_offset;
        uint64_t filesz      = ph[i].p_filesz;
        uint64_t memsz       = ph[i].p_memsz;
        uint64_t page_base   = seg_vaddr & ~0xFFFULL;
        uint64_t page_offset = seg_vaddr & 0xFFFULL;
        uint64_t to_cover    = page_offset + memsz;
        uint64_t pages       = (to_cover + 0xFFF) / 0x1000;

        uint64_t flags = ENTRY_FLAG_PRESENT | ENTRY_FLAG_USER;
        if (ph[i].p_flags & PF_W)
            flags |= ENTRY_FLAG_RW;
        if (!(ph[i].p_flags & PF_X))
            flags |= ENTRY_FLAG_NX;

        for (uint64_t p = 0; p < pages; p++) {
            uint64_t va   = page_base + (p * 0x1000);
            paddr    phys = frame_alloc();
            paging_map_page(pml4, va, phys, 0x1000, flags);

            void *dst = (void *)(offset + phys);
            memset(dst, 0, 0x1000);

            if (va + 0x1000 > seg_vaddr && filesz > 0) {
                uint64_t seg_file_off = (va < seg_vaddr) ? 0 : (va - seg_vaddr);
                if (seg_file_off < filesz) {
                    uint64_t avail = filesz - seg_file_off;
                    uint64_t copy  = avail > 0x1000 ? 0x1000 : avail;
                    memcpy(
                        (uint8_t *)dst + ((va < seg_vaddr) ? (seg_vaddr - va) : 0),
                        (uint8_t *)elf + seg_offset + seg_file_off,
                        copy
                    );
                }
            }
        }
    }

    if (info) {
        info->phdr_vaddr  = phdr_vaddr;
        info->phent       = eh->e_phentsize;
        info->phnum       = eh->e_phnum;
        info->entry       = eh->e_entry;
        info->interp_base = 0;
        info->vdso_base   = NULL;
        info->execfn      = NULL;
    }

    uint64_t entry = eh->e_entry;
    kfree(elf);
    return entry;
}