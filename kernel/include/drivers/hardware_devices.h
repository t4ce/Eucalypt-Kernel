#pragma once

#include <stdint.h>

typedef enum {
    STORAGE_DEVICE_AHCI = 0,
    STORAGE_DEVICE_NVME = 1,
    STORAGE_DEVICE_IDE  = 2
} storage_device_type_t;

typedef struct storage_device {
    storage_device_type_t type;
    void *device;
    int sata_controller_id; // -1 if not applicable
    int sata_port_id;       // -1 if not applicable
    int ide_drive_num;      // -1 if not applicable
    uint8_t (*write)(uint8_t port, uint8_t drive, uint64_t sector, uint32_t count, const void *buf);
    uint8_t (*read)(uint8_t port, uint8_t drive, uint64_t sector, uint32_t count, void *buf);
} storage_device_t;

typedef struct {
    uint8_t ahci_count;
    uint8_t nvme_count;
    uint8_t ide_count;
    uint8_t total_count;
} storage_device_counts_t;

void                    storage_device_init();
uint8_t                 get_total_drive_count();
uint8_t                 get_ahci_drive_count();
uint8_t                 get_nvme_drive_count();
uint8_t                 get_ide_drive_count();
storage_device_counts_t get_storage_device_counts();
storage_device_t        *get_storage_device(storage_device_type_t type, uint8_t index);
storage_device_t        *get_storage_device_by_index(uint8_t index);