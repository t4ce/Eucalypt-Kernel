#include <stdint.h>
#include <stdbool.h>
#include <limine.h>
#include <logging/flanterm.h>
#include <logging/printk.h>
#include <gdt/gdt.h>
#include <idt/idt.h>
#include <mm/hhdm.h>
#include <mm/frame.h>
#include <mm/paging.h>
#include <mm/vm.h>
#include <mm/heap.h>
#include <mm/types.h>
#include <binl/elf64.h>
#include <auxv.h>
#include <interrupts/apic.h>
#include <multitasking/thread.h>
#include <multitasking/sched.h>
#include <multitasking/proc.h>
#include <drivers/pci.h>
#include <drivers/block/ahci.h>
#include <drivers/block/ramfs.h>
#include <drivers/fs/vfs/vfs.h>
#include <drivers/fs/devfs/devfs.h>
#include <drivers/tty.h>

extern void enable_sse();

__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(6);

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

static void hcf(void) {
    for (;;) {
#if defined (__x86_64__)
        asm ("hlt");
#elif defined (__aarch64__) || defined (__riscv)
        asm ("wfi");
#elif defined (__loongarch64)
        asm ("idle 0");
#endif
    }
}

[[gnu::noreturn]]
void idle_thread(void) {
    while (1) {
        __asm__ volatile("hlt");
    }
}

uint64_t alloc_user_stack(uint64_t *cr3) {
    uint64_t user_stack_base = 0x70000000000;
    uint64_t pages = 4;
    uint64_t flags =
        ENTRY_FLAG_PRESENT |
        ENTRY_FLAG_RW      |
        ENTRY_FLAG_NX      |
        ENTRY_FLAG_USER;

    for (uint64_t i = 0; i < pages; i++) {
        paddr frame = frame_alloc();
        vaddr virt  = user_stack_base + (i * 0x1000);
        paging_map_page(cr3, virt, frame, 0x1000, flags);
    }

    return user_stack_base + (pages * 0x1000);
}

extern struct flanterm_context *ft_ctx;

void putchar(tty_t *tty, char c) {
    (void)tty;
    flanterm_write(ft_ctx, &c, 1);
}

void kmain(void) {
    if (!LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision)) {
        hcf();
    }

    printk_init();
    gdt_init();
    log_info("GDT initialized\n");
    idt_init();
    log_info("IDT initialized\n");
    hhdm_init();
    log_info("HHDM initialized\n");
    frame_init();
    log_info("Frame allocator initialized\n");
    paging_init();
    log_info("Paging initialized\n");
    heap_init();
    log_info("Heap initialized\n");
    enable_apic(true);
    log_info("APIC enabled\n");
    ahci_init();
    log_info("AHCI initialized\n");
    enable_sse();
    vfs_init();
    log_info("VFS initialized\n");
    tty_init(putchar);
    ramfs_init();

    if (!ramfs_addr || ramfs_size == 0) {
        log_error("No ramfs module loaded\n");
    } else {
        uint64_t capacity = ramfs_size + (1024 * 1024);
        uint8_t  mret     = ramfs_mount(ramfs_addr, ramfs_size, capacity);
        if (mret != VFS_OK) {
            log_error("Failed to mount ramfs: %d\n", mret);
            hcf();
        }
        log_info("Ramfs mounted\n");
    }

    scheduler_init();

    paddr idle_cr3 = paging_create_pml4();
    create_thread(idle_thread, idle_cr3);
    log_info("Idle thread created\n");

    // int fd = vfs_open("/ram/USER", VFS_O_RDONLY);
    // if (fd < 0) {
    //     log_error("Failed to open /ram/USER: %d\n", fd);
    //     hcf();
    // }

    // paddr user_cr3 = paging_create_pml4();

    // elf_load_info_t info = {0};
    // uint64_t entry = elf64_parse(fd, user_cr3, &info);
    // vfs_close(fd);

    // if (!entry) {
    //     log_error("Failed to load ELF64 binary\n");
    //     hcf();
    // }

    // info.execfn = "/ram/USER";
    // 
    // log_info("Creating user thread: entry=%llx cr3=%llx\n", entry, user_cr3);
    // 
    // char *argv[] = { "/ram/USER", NULL };
    // char *envp[] = { "PATH=/", NULL };
    // 
    // create_user_thread_with_stack(entry, user_cr3, argv, envp, &info);

    enable_sched();
    log_info("Scheduler enabled");
    apic_timer_init(1000);
    log_info("APIC timer initialized");
    __asm__ volatile("sti");

    for (;;) {
        __asm__ volatile("hlt");
    }
}