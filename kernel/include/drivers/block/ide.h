#pragma once

#include <stdint.h>

#define IDE_MAX_DRIVES      4
#define IDE_SECTOR_SIZE     512

typedef struct {
    uint8_t type;
    uint8_t present;
    uint8_t number;
    char assigned_letter;
} ide_drive_info_t;

typedef struct {
    ide_drive_info_t drives[IDE_MAX_DRIVES];
} ide_state_t;

extern uint8_t ide_count;

uint8_t ide_read(uint8_t port, uint8_t drive, uint64_t sector, uint8_t count, void *buf);
uint8_t ide_write(uint8_t port, uint8_t drive, uint64_t sector, uint8_t count, const void *buf);
uint8_t ide_drive_present(uint8_t port, uint8_t drive);
char ide_drive_letter(uint8_t port, uint8_t drive);
void ide_set_drive_letter(uint8_t port, uint8_t drive, char letter);
ide_drive_info_t *ide_get_device(uint8_t index);
uint8_t ide_init(uint32_t bar0, uint32_t bar1, uint32_t bar2, uint32_t bar3, uint32_t bar4);
