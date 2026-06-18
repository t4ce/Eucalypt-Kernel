#include <stdint.h>
#include <drivers/block/ahci.h>
#include <drivers/block/nvme.h>
#include <drivers/block/ide.h>
#include <logging/printk.h>
#include <mm/heap.h>
#include <drivers/hardware_devices.h>

struct storage_device_node {
    storage_device_t *device;
    struct storage_device_node *next;
};

typedef struct {
    struct storage_device_node *head;
    struct storage_device_node *tail;
} all_devices_t;

static storage_device_counts_t device_counts = {0};
static all_devices_t all_devices = {0};
static uint8_t initialized = 0;

static void append_device(storage_device_t *dev) {
    struct storage_device_node *node = kmalloc(sizeof(struct storage_device_node));
    if (!node) return;

    node->device = dev;
    node->next = NULL;

    if (all_devices.head == NULL) {
        all_devices.head = node;
        all_devices.tail = node;
    } else {
        all_devices.tail->next = node;
        all_devices.tail = node;
    }
}

static uint8_t ide_write_wrapper(uint8_t port, uint8_t drive, uint64_t sector, uint32_t count, const void *buf) {
    return ide_write(port, drive, sector, (uint32_t)count, buf);
}

static uint8_t ide_read_wrapper(uint8_t port, uint8_t drive, uint64_t sector, uint32_t count, void *buf) {
    return ide_read(port, drive, (uint32_t)sector, count, buf);
}

static uint8_t ahci_write_wrapper(uint8_t controller, uint8_t port, uint64_t sector, uint32_t count, const void *buf) {
    return ahci_write(controller, port, sector, count, buf);
}

static uint8_t ahci_read_wrapper(uint8_t controller, uint8_t port, uint64_t sector, uint32_t count, void *buf) {
    return ahci_read(controller, port, sector, count, buf);
}

void storage_device_init(void) {
    if (initialized) return;

    device_counts.ahci_count = ahci_get_controller_count();
    device_counts.nvme_count = nvme_get_controller_count();
    device_counts.ide_count  = ide_count;

    device_counts.total_count = device_counts.ahci_count +
                                 device_counts.nvme_count +
                                 device_counts.ide_count;

    if (device_counts.ide_count > 0) {
        for (uint8_t i = 0; i < device_counts.ide_count; i++) {
            storage_device_t *dev = kmalloc(sizeof(storage_device_t));
            if (!dev) {
                log_error("Failed to allocate heap for IDE device\n");
                continue;
            }

            dev->type               = STORAGE_DEVICE_IDE;
            dev->device             = ide_get_device(i);
            dev->sata_controller_id = -1;
            dev->sata_port_id       = -1;
            dev->ide_drive_num      = i;
            dev->read               = ide_read_wrapper;
            dev->write              = ide_write_wrapper;

            append_device(dev);
        }
    }

    if (device_counts.ahci_count > 0) {
        for (uint8_t c = 0; c < device_counts.ahci_count; c++) {
            uint8_t port_count = ahci_get_port_count(c);
    
            for (uint8_t p = 0; p < port_count; p++) {
                uint8_t port_idx = ahci_get_port_index(c, p);
                if (port_idx == 0xFF)
                    continue;
    
                storage_device_t *dev = kmalloc(sizeof(storage_device_t));
                if (!dev) {
                    log_error("Failed to allocate heap for AHCI device c%u p%u\n", c, port_idx);
                    continue;
                }
    
                dev->type               = STORAGE_DEVICE_AHCI;
                dev->device             = ahci_get_controller(c);
                dev->sata_controller_id = c;
                dev->sata_port_id       = port_idx;
                dev->ide_drive_num      = -1;
                dev->read               = ahci_read_wrapper;
                dev->write              = ahci_write_wrapper;
    
                append_device(dev);
            }
        }
    }

    if (device_counts.nvme_count > 0) {
        for (uint8_t i = 0; i < device_counts.nvme_count; i++) {
            // I don't have full NVMe suppor yet
        }
    }

    uint8_t total = 0;
    for (struct storage_device_node *n = all_devices.head; n; n = n->next)
        total++;
    device_counts.total_count = total;

    initialized = 1;
}

uint8_t get_total_drive_count(void) {
    if (!initialized) storage_device_init();
    return device_counts.total_count;
}

uint8_t get_ahci_drive_count(void) {
    if (!initialized) storage_device_init();
    return device_counts.ahci_count;
}

uint8_t get_nvme_drive_count(void) {
    if (!initialized) storage_device_init();
    return device_counts.nvme_count;
}

uint8_t get_ide_drive_count(void) {
    if (!initialized) storage_device_init();
    return device_counts.ide_count;
}

storage_device_counts_t get_storage_device_counts(void) {
    if (!initialized) storage_device_init();
    return device_counts;
}

storage_device_t *get_storage_device(storage_device_type_t type, uint8_t index) {
    if (!initialized) storage_device_init();

    struct storage_device_node *node = all_devices.head;
    uint8_t count = 0;

    while (node != NULL) {
        if (node->device->type == type) {
            if (count == index) return node->device;
            count++;
        }
        node = node->next;
    }

    return NULL;
}

storage_device_t *get_storage_device_by_index(uint8_t index) {
    if (!initialized) storage_device_init();

    struct storage_device_node *node = all_devices.head;
    uint8_t count = 0;

    while (node != NULL) {
        if (count == index) return node->device;
        count++;
        node = node->next;
    }

    return NULL;
}