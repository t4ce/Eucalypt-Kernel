# Eucalypt-Kernel

A modern x86-64 kernel written in C and assembly with support for multitasking, virtual memory, and signal handling.

## Features

- x86-64 architecture support
- Preemptive multitasking and scheduling
- Virtual memory with paging
- Signal delivery and handling
- FAT16 filesystem support
- ELF64 binary loading
- APIC and IOAPIC interrupt handling
- PS/2 keyboard and mouse support
- Basic TTY driver

## Building

install block

```bash
    block --file build.block --target iso
```

## Running

```bash
block --file build.block --target run
```

## API

### Syscalls numbers
