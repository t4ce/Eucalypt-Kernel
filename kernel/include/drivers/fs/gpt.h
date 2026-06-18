#pragma once

#include <stdint.h>
#include <drivers/hardware_devices.h>

typedef struct __attribute__((packed)) partition_entry {
    uint8_t  type_guid[16];
    uint8_t  unique_guid[16];
    uint64_t lba;
    uint64_t last_lba;
    uint64_t attributes;
    uint8_t  name[72];
} partition_entry_t;

uint8_t gpt_write_partition(storage_device_t *dev, uint32_t slot, struct partition_entry *entry);
uint8_t gpt_init();