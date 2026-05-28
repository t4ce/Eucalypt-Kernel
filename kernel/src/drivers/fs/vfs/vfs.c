#include <stdint.h>
#include <stddef.h>
#include <mem.h>
#include <mm/heap.h>
#include <drivers/block/ahci.h>
#include <drivers/block/nvme.h>
#include <drivers/block/ide.h>
#include <drivers/fs/vfs/blockdev.h>
#include <drivers/fs/fat16/fat16.h>
#include <drivers/fs/devfs/devfs.h>
#include <drivers/fs/vfs/vfs.h>

#define MAX_DRIVES      254
#define MAX_FD          256
#define MAX_PATH        4096
#define MAX_NAME        256

static vfs_mount_t  mount_table[MAX_DRIVES];
static uint8_t      mount_count = 0;
static uint8_t      vfs_ready   = 0;

static vfs_file_t   fd_table[MAX_FD];
static uint8_t      fd_ready = 0;

static vfs_node_t  *vfs_root = NULL;

static void fd_table_init(void) {
    for (int i = 0; i < MAX_FD; i++) {
        fd_table[i].node   = NULL;
        fd_table[i].offset = 0;
        fd_table[i].flags  = 0;
        fd_table[i].open   = 0;
    }
    fd_ready = 1;
}

static int fd_alloc(void) {
    for (int i = 0; i < MAX_FD; i++) {
        if (!fd_table[i].open) {
            return i;
        }
    }
    return -1;
}

vfs_node_t *vfs_node_alloc(const char *name, uint32_t type) {
    vfs_node_t *node = kmalloc(sizeof(vfs_node_t));
    if (!node) {
        return NULL;
    }
    memset(node, 0, sizeof(vfs_node_t));
    size_t len = strlen(name);
    if (len >= MAX_NAME) len = MAX_NAME - 1;
    memcpy(node->name, name, len);
    node->name[len] = '\0';
    node->type      = type;
    node->ref_count = 0;
    node->parent    = NULL;
    node->children  = NULL;
    node->next      = NULL;
    node->ops       = NULL;
    node->priv      = NULL;
    return node;
}

void vfs_node_link_child(vfs_node_t *parent, vfs_node_t *child) {
    child->parent = parent;
    child->next   = parent->children;
    parent->children = child;
}

void vfs_node_unlink_child(vfs_node_t *parent, vfs_node_t *child) {
    vfs_node_t **cur = &parent->children;
    while (*cur) {
        if (*cur == child) {
            *cur = child->next;
            child->next   = NULL;
            child->parent = NULL;
            return;
        }
        cur = &(*cur)->next;
    }
}

vfs_node_t *vfs_node_find_child(vfs_node_t *parent, const char *name) {
    for (vfs_node_t *c = parent->children; c; c = c->next) {
        if (strcmp(c->name, name) == 0) {
            return c;
        }
    }
    return NULL;
}

vfs_node_t *vfs_resolve_path(const char *path) {
    if (!path || !vfs_root) return NULL;

    if (path[0] != '/') return NULL;

    vfs_node_t *node = vfs_root;
    const char *p    = path + 1;

    while (*p) {
        if (*p == '/') {
            p++;
            continue;
        }
        char component[MAX_NAME];
        size_t len = 0;
        while (*p && *p != '/') {
            if (len < MAX_NAME - 1) component[len++] = *p;
            p++;
        }
        component[len] = '\0';

        if (strcmp(component, ".") == 0) continue;
        if (strcmp(component, "..") == 0) {
            if (node->parent) node = node->parent;
            continue;
        }

        if (node->type == VFS_NODE_MOUNTPOINT) {
            vfs_node_t *inner = (vfs_node_t *)node->priv;
            if (inner) node = inner;
        }

        vfs_node_t *child = NULL;

        if (node->ops && node->ops->lookup) {
            child = node->ops->lookup(node, component);
        } else {
            child = vfs_node_find_child(node, component);
        }

        if (!child) return NULL;
        node = child;
    }

    return node;
}

vfs_node_t *vfs_resolve_parent(const char *path, char *name_out) {
    if (!path || path[0] != '/') return NULL;

    char buf[MAX_PATH];
    size_t len = strlen(path);
    if (len >= MAX_PATH) return NULL;
    memcpy(buf, path, len + 1);

    char *last_slash = buf;
    for (char *p = buf; *p; p++) {
        if (*p == '/') last_slash = p;
    }

    if (name_out) {
        size_t nlen = strlen(last_slash + 1);
        if (nlen >= MAX_NAME) nlen = MAX_NAME - 1;
        memcpy(name_out, last_slash + 1, nlen);
        name_out[nlen] = '\0';
    }

    if (last_slash == buf) {
        return vfs_root;
    }

    *last_slash = '\0';
    return vfs_resolve_path(buf);
}

static int find_letter_slot(char letter) {
    for (int i = 0; i < mount_count; i++) {
        if (mount_table[i].letter == letter) return i;
    }
    return -1;
}

vfs_mount_t *vfs_get_mount(char letter) {
    int slot = find_letter_slot(letter);
    if (slot < 0) return NULL;
    return &mount_table[slot];
}

static uint8_t ahci_blockdev_read(vfs_blockdev_t *dev, uint32_t lba, uint8_t count, void *buf) {
    vfs_blockdev_priv_t *priv = (vfs_blockdev_priv_t *)dev->priv;
    return ahci_read(priv->addr.ahci.controller, priv->addr.ahci.port, lba, count, buf);
}

static uint8_t ahci_blockdev_write(vfs_blockdev_t *dev, uint32_t lba, uint8_t count, const void *buf) {
    vfs_blockdev_priv_t *priv = (vfs_blockdev_priv_t *)dev->priv;
    return ahci_write(priv->addr.ahci.controller, priv->addr.ahci.port, lba, count, buf);
}

static uint8_t nvme_blockdev_read(vfs_blockdev_t *dev, uint32_t lba, uint8_t count, void *buf) {
    vfs_blockdev_priv_t *priv = (vfs_blockdev_priv_t *)dev->priv;
    return nvme_read(priv->addr.nvme.controller, priv->addr.nvme.nsid, lba, count, buf);
}

static uint8_t nvme_blockdev_write(vfs_blockdev_t *dev, uint32_t lba, uint8_t count, const void *buf) {
    vfs_blockdev_priv_t *priv = (vfs_blockdev_priv_t *)dev->priv;
    return nvme_write(priv->addr.nvme.controller, priv->addr.nvme.nsid, lba, count, buf);
}

static uint8_t ide_blockdev_read(vfs_blockdev_t *dev, uint32_t lba, uint8_t count, void *buf) {
    vfs_blockdev_priv_t *priv = (vfs_blockdev_priv_t *)dev->priv;
    return ide_read(priv->addr.ide.bus, priv->addr.ide.drive, lba, count, buf);
}

static uint8_t ide_blockdev_write(vfs_blockdev_t *dev, uint32_t lba, uint8_t count, const void *buf) {
    vfs_blockdev_priv_t *priv = (vfs_blockdev_priv_t *)dev->priv;
    return ide_write(priv->addr.ide.bus, priv->addr.ide.drive, lba, count, buf);
}

fs_t vfs_get_type(vfs_blockdev_t *blockdev) {
    uint8_t buf[512];
    if (blockdev->read(blockdev, 0, 1, buf) != 0) return (fs_t)-1;
    if (memcmp(buf + 0x36, "FAT12   ", 8) == 0) return fat12;
    if (memcmp(buf + 0x36, "FAT16   ", 8) == 0) return fat16;
    if (memcmp(buf + 0x52, "FAT32   ", 8) == 0) return fat32;
    if (memcmp(buf + 0x03, "EXFAT   ", 8) == 0) return exfat;
    return (fs_t)-1;
}

static int32_t fat16_vfs_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buf) {
    vfs_fat16_priv_t *priv = (vfs_fat16_priv_t *)node->priv;
    if (!priv) return -1;
    if (offset >= priv->size) return 0;
    uint32_t remaining = priv->size - offset;
    if (size > remaining) size = remaining;

    uint8_t *tmp = kmalloc(priv->size);
    if (!tmp) return -1;

    uint32_t bytes_read = 0;
    if (fat16_read_file(priv->vol, priv->start_cluster, priv->size, tmp, &bytes_read) != 0) {
        kfree(tmp);
        return -1;
    }

    memcpy(buf, tmp + offset, size);
    kfree(tmp);
    return (int32_t)size;
}

static int32_t fat16_vfs_write(vfs_node_t *node, uint32_t offset, uint32_t size, const uint8_t *buf) {
    vfs_fat16_priv_t *priv = (vfs_fat16_priv_t *)node->priv;
    if (!priv) return -1;

    uint32_t new_size = (offset + size > priv->size) ? offset + size : priv->size;
    uint8_t *tmp = kmalloc(new_size);
    if (!tmp) return -1;
    memset(tmp, 0, new_size);

    if (priv->size > 0) {
        uint32_t bytes_read = 0;
        fat16_read_file(priv->vol, priv->start_cluster, priv->size, tmp, &bytes_read);
    }

    memcpy(tmp + offset, buf, size);

    if (priv->start_cluster) {
        fat16_delete_file(priv->vol, priv->dir_cluster, node->name);
        priv->start_cluster = 0;
    }

    uint32_t bytes_written = 0;
    uint16_t new_cluster = priv->start_cluster;
    if (fat16_write_file(priv->vol, &new_cluster, tmp, new_size, &bytes_written) != 0) {
        kfree(tmp);
        return -1;
    }

    if (fat16_create_dirent_update(priv->vol, priv->dir_cluster, node->name, new_cluster, new_size) != 0) {
        kfree(tmp);
        return -1;
    }

    priv->start_cluster = new_cluster;
    priv->size          = new_size;
    node->size          = new_size;
    kfree(tmp);
    return (int32_t)bytes_written;
}

static int fat16_vfs_readdir(vfs_node_t *node, uint32_t index, vfs_dirent_t *out) {
    vfs_fat16_priv_t *priv = (vfs_fat16_priv_t *)node->priv;
    if (!priv) return -1;

    fat16_dir_entry entries[512];
    uint16_t count = 512;
    if (fat16_list_directory(priv->vol, priv->start_cluster, entries, &count) != 0) return -1;
    if (index >= count) return -1;

    size_t nlen = strlen(entries[index].name);
    if (nlen >= MAX_NAME) nlen = MAX_NAME - 1;
    memcpy(out->name, entries[index].name, nlen);
    out->name[nlen] = '\0';
    out->type = (entries[index].attr & 0x10) ? VFS_NODE_DIR : VFS_NODE_FILE;
    return 0;
}

static vfs_node_t *fat16_vfs_lookup(vfs_node_t *dir, const char *name) {
    vfs_node_t *existing = vfs_node_find_child(dir, name);
    if (existing) return existing;

    vfs_fat16_priv_t *dpriv = (vfs_fat16_priv_t *)dir->priv;
    if (!dpriv) return NULL;

    fat16_file_handle handle;
    if (fat16_find_file(dpriv->vol, dpriv->start_cluster, name, &handle) != 0) return NULL;

    uint32_t type = (handle.attr & 0x10) ? VFS_NODE_DIR : VFS_NODE_FILE;
    vfs_node_t *child = vfs_node_alloc(name, type);
    if (!child) return NULL;

    vfs_fat16_priv_t *cpriv = kmalloc(sizeof(vfs_fat16_priv_t));
    if (!cpriv) { kfree(child); return NULL; }

    cpriv->vol          = dpriv->vol;
    cpriv->start_cluster = handle.start_cluster;
    cpriv->size         = handle.size;
    cpriv->dir_cluster  = dpriv->start_cluster;
    child->priv         = cpriv;
    child->size         = handle.size;
    child->ops          = dir->ops;

    vfs_node_link_child(dir, child);
    return child;
}

static int fat16_vfs_create(vfs_node_t *dir, const char *name, uint32_t type) {
    vfs_fat16_priv_t *dpriv = (vfs_fat16_priv_t *)dir->priv;
    if (!dpriv) return -1;

    if (type == VFS_NODE_DIR) {
        return (fat16_create_directory(dpriv->vol, dpriv->start_cluster, name) == 0) ? 0 : -1;
    } else {
        return (fat16_create_file(dpriv->vol, dpriv->start_cluster, name, NULL, 0) == 0) ? 0 : -1;
    }
}

static int fat16_vfs_unlink(vfs_node_t *dir, const char *name) {
    vfs_fat16_priv_t *dpriv = (vfs_fat16_priv_t *)dir->priv;
    if (!dpriv) return -1;
    return (fat16_delete_file(dpriv->vol, dpriv->start_cluster, name) == 0) ? 0 : -1;
}

static int fat16_vfs_rmdir(vfs_node_t *dir, const char *name) {
    vfs_fat16_priv_t *dpriv = (vfs_fat16_priv_t *)dir->priv;
    if (!dpriv) return -1;
    return (fat16_delete_directory(dpriv->vol, dpriv->start_cluster, name) == 0) ? 0 : -1;
}

static vfs_node_ops_t fat16_ops = {
    .read    = fat16_vfs_read,
    .write   = fat16_vfs_write,
    .readdir = fat16_vfs_readdir,
    .lookup  = fat16_vfs_lookup,
    .create  = fat16_vfs_create,
    .unlink  = fat16_vfs_unlink,
    .rmdir   = fat16_vfs_rmdir,
};

uint8_t vfs_mount(StorageDevType dev, char letter, vfs_dev_addr_t addr) {
    if (find_letter_slot(letter) >= 0) return VFS_ERR_LETTER_IN_USE;
    if (mount_count >= MAX_DRIVES)     return VFS_ERR_NO_SLOTS;

    vfs_blockdev_priv_t *priv = kmalloc(sizeof(vfs_blockdev_priv_t));
    if (!priv) return VFS_ERR_NO_SLOTS;
    priv->addr = addr;

    vfs_blockdev_t blockdev = {0};
    blockdev.priv = priv;

    if (dev == AHCI) {
        uint8_t controller = addr.ahci.controller;
        uint8_t port       = addr.ahci.port;
        if (controller >= ahci_get_controller_count()) { kfree(priv); return VFS_ERR_INVALID_DEV; }
        ahci_controller_t *c = ahci_get_controller(controller);
        if (!c || !c->ports[port].present)              { kfree(priv); return VFS_ERR_INVALID_DEV; }
        if (c->ports[port].assigned_letter != 0)        { kfree(priv); return VFS_ERR_ALREADY_MOUNTED; }
        blockdev.read  = ahci_blockdev_read;
        blockdev.write = ahci_blockdev_write;

    } else if (dev == NVME) {
        uint8_t  controller = addr.nvme.controller;
        uint32_t nsid       = addr.nvme.nsid;
        if (controller >= nvme_get_controller_count()) { kfree(priv); return VFS_ERR_INVALID_DEV; }
        nvme_controller_t *c = nvme_get_controller(controller);
        if (!c || !nvme_namespace_present(c, nsid))    { kfree(priv); return VFS_ERR_INVALID_DEV; }
        if (nvme_namespace_letter(c, nsid) != 0)       { kfree(priv); return VFS_ERR_ALREADY_MOUNTED; }
        blockdev.read  = nvme_blockdev_read;
        blockdev.write = nvme_blockdev_write;

    } else if (dev == IDE) {
        uint8_t bus   = addr.ide.bus;
        uint8_t drive = addr.ide.drive;
        if (bus > 1 || drive > 1)          { kfree(priv); return VFS_ERR_INVALID_DEV; }
        if (!ide_drive_present(bus, drive)) { kfree(priv); return VFS_ERR_INVALID_DEV; }
        if (ide_drive_letter(bus, drive))   { kfree(priv); return VFS_ERR_ALREADY_MOUNTED; }
        blockdev.read  = ide_blockdev_read;
        blockdev.write = ide_blockdev_write;

    } else {
        kfree(priv);
        return VFS_ERR_UNKNOWN_DEV;
    }

    fs_t type = vfs_get_type(&blockdev);
    if (type != fat16) { kfree(priv); return VFS_ERR_FS_INIT; }

    void *vol_ptr = fat16_init(&blockdev);
    if (!vol_ptr)  { kfree(priv); return VFS_ERR_FS_INIT; }

    char mnt_name[3] = { letter, ':', '\0' };
    vfs_node_t *mp_outer = vfs_node_alloc(mnt_name, VFS_NODE_MOUNTPOINT);
    if (!mp_outer) { kfree(priv); return VFS_ERR_NO_SLOTS; }

    vfs_fat16_priv_t *fat_priv = kmalloc(sizeof(vfs_fat16_priv_t));
    if (!fat_priv) { kfree(mp_outer); kfree(priv); return VFS_ERR_NO_SLOTS; }
    fat_priv->vol          = vol_ptr;
    fat_priv->start_cluster = 0;
    fat_priv->size          = 0;
    fat_priv->dir_cluster   = 0;

    vfs_node_t *root_dir = vfs_node_alloc("/", VFS_NODE_DIR);
    if (!root_dir) { kfree(fat_priv); kfree(mp_outer); kfree(priv); return VFS_ERR_NO_SLOTS; }
    root_dir->priv = fat_priv;
    root_dir->ops  = &fat16_ops;

    mp_outer->priv = root_dir;
    vfs_node_link_child(vfs_root, mp_outer);

    mount_table[mount_count].letter   = letter;
    mount_table[mount_count].blockdev = blockdev;
    mount_table[mount_count].dev      = dev;
    mount_table[mount_count].addr     = addr;
    mount_table[mount_count].priv     = vol_ptr;
    mount_count++;

    if (dev == AHCI) {
        ahci_controller_t *c = ahci_get_controller(addr.ahci.controller);
        if (c) c->ports[addr.ahci.port].assigned_letter = letter;
    } else if (dev == NVME) {
        nvme_controller_t *c = nvme_get_controller(addr.nvme.controller);
        if (c) nvme_set_namespace_letter(c, addr.nvme.nsid, letter);
    } else if (dev == IDE) {
        ide_set_drive_letter(addr.ide.bus, addr.ide.drive, letter);
    }

    return VFS_OK;
}

void vfs_unmount(char letter) {
    int slot = find_letter_slot(letter);
    if (slot < 0) return;

    vfs_mount_t *m = &mount_table[slot];
    vfs_blockdev_priv_t *bpriv = (vfs_blockdev_priv_t *)m->blockdev.priv;

    if (m->dev == AHCI) {
        ahci_controller_t *c = ahci_get_controller(m->addr.ahci.controller);
        if (c) c->ports[m->addr.ahci.port].assigned_letter = 0;
    } else if (m->dev == NVME) {
        nvme_controller_t *c = nvme_get_controller(m->addr.nvme.controller);
        if (c) nvme_set_namespace_letter(c, m->addr.nvme.nsid, 0);
    } else if (m->dev == IDE) {
        ide_set_drive_letter(m->addr.ide.bus, m->addr.ide.drive, 0);
    }

    char mnt_name[3] = { letter, ':', '\0' };
    vfs_node_t *mp = vfs_node_find_child(vfs_root, mnt_name);
    if (mp) {
        vfs_node_unlink_child(vfs_root, mp);
        kfree(mp);
    }

    if (bpriv) kfree(bpriv);

    for (int i = slot; i < mount_count - 1; i++) mount_table[i] = mount_table[i + 1];
    mount_count--;
}

int vfs_open(const char *path, uint32_t flags) {
    if (!vfs_ready || !fd_ready) return -1;

    vfs_node_t *node = vfs_resolve_path(path);

    if (!node) {
        if (!(flags & VFS_O_CREAT)) return -1;

        char name[MAX_NAME];
        vfs_node_t *parent = vfs_resolve_parent(path, name);
        if (!parent) return -1;

        if (parent->ops && parent->ops->create) {
            if (parent->ops->create(parent, name, VFS_NODE_FILE) != 0) return -1;
        } else {
            vfs_node_t *new_node = vfs_node_alloc(name, VFS_NODE_FILE);
            if (!new_node) return -1;
            vfs_node_link_child(parent, new_node);
        }

        node = vfs_resolve_path(path);
        if (!node) return -1;
    }

    if ((flags & VFS_O_TRUNC) && node->type == VFS_NODE_FILE) {
        if (node->ops && node->ops->write) {
            node->ops->write(node, 0, 0, NULL);
        }
        node->size = 0;
    }

    int fd = fd_alloc();
    if (fd < 0) return -1;

    fd_table[fd].node   = node;
    fd_table[fd].offset = (flags & VFS_O_APPEND) ? node->size : 0;
    fd_table[fd].flags  = flags;
    fd_table[fd].open   = 1;
    node->ref_count++;

    return fd;
}

int vfs_close(int fd) {
    if (fd < 0 || fd >= MAX_FD || !fd_table[fd].open) return -1;
    fd_table[fd].node->ref_count--;
    fd_table[fd].node   = NULL;
    fd_table[fd].offset = 0;
    fd_table[fd].flags  = 0;
    fd_table[fd].open   = 0;
    return 0;
}

int32_t vfs_read(int fd, void *buf, uint32_t count) {
    if (fd < 0 || fd >= MAX_FD || !fd_table[fd].open) return -1;
    if (!buf || count == 0) return 0;

    vfs_node_t *node = fd_table[fd].node;
    if (!node) return -1;

    if (node->type == VFS_NODE_DEV) {
        devfs_dev_t *ddev = (devfs_dev_t *)node->priv;
        if (ddev && ddev->read) {
            int32_t n = ddev->read(ddev, buf, count);
            if (n > 0) fd_table[fd].offset += n;
            return n;
        }
        return -1;
    }

    if (!node->ops || !node->ops->read) return -1;
    int32_t n = node->ops->read(node, fd_table[fd].offset, count, (uint8_t *)buf);
    if (n > 0) fd_table[fd].offset += n;
    return n;
}

int32_t vfs_write(int fd, const void *buf, uint32_t count) {
    if (fd < 0 || fd >= MAX_FD || !fd_table[fd].open) return -1;
    if (!buf || count == 0) return 0;

    vfs_node_t *node = fd_table[fd].node;
    if (!node) return -1;

    if (node->type == VFS_NODE_DEV) {
        devfs_dev_t *ddev = (devfs_dev_t *)node->priv;
        if (ddev && ddev->write) {
            int32_t n = ddev->write(ddev, buf, count);
            if (n > 0) fd_table[fd].offset += n;
            return n;
        }
        return -1;
    }

    if (!node->ops || !node->ops->write) return -1;
    int32_t n = node->ops->write(node, fd_table[fd].offset, count, (const uint8_t *)buf);
    if (n > 0) fd_table[fd].offset += n;
    return n;
}

int32_t vfs_seek(int fd, int32_t offset, int whence) {
    if (fd < 0 || fd >= MAX_FD || !fd_table[fd].open) return -1;
    vfs_node_t *node = fd_table[fd].node;
    if (!node) return -1;

    int32_t new_offset;
    switch (whence) {
        case VFS_SEEK_SET: new_offset = offset; break;
        case VFS_SEEK_CUR: new_offset = (int32_t)fd_table[fd].offset + offset; break;
        case VFS_SEEK_END: new_offset = (int32_t)node->size + offset; break;
        default: return -1;
    }

    if (new_offset < 0) return -1;
    fd_table[fd].offset = (uint32_t)new_offset;
    return new_offset;
}

int vfs_stat(const char *path, vfs_stat_t *st) {
    if (!st) return -1;
    vfs_node_t *node = vfs_resolve_path(path);
    if (!node) return -1;

    st->type  = node->type;
    st->size  = node->size;
    st->flags = node->flags;
    size_t nlen = strlen(node->name);
    if (nlen >= MAX_NAME) nlen = MAX_NAME - 1;
    memcpy(st->name, node->name, nlen);
    st->name[nlen] = '\0';
    return 0;
}

int vfs_fstat(int fd, vfs_stat_t *st) {
    if (fd < 0 || fd >= MAX_FD || !fd_table[fd].open || !st) return -1;
    vfs_node_t *node = fd_table[fd].node;
    if (!node) return -1;

    st->type  = node->type;
    st->size  = node->size;
    st->flags = node->flags;
    size_t nlen = strlen(node->name);
    if (nlen >= MAX_NAME) nlen = MAX_NAME - 1;
    memcpy(st->name, node->name, nlen);
    st->name[nlen] = '\0';
    return 0;
}

int vfs_mkdir(const char *path) {
    char name[MAX_NAME];
    vfs_node_t *parent = vfs_resolve_parent(path, name);
    if (!parent) return -1;

    if (parent->type == VFS_NODE_MOUNTPOINT) {
        vfs_node_t *inner = (vfs_node_t *)parent->priv;
        if (inner) parent = inner;
    }

    if (parent->ops && parent->ops->create) {
        return parent->ops->create(parent, name, VFS_NODE_DIR);
    }

    vfs_node_t *node = vfs_node_alloc(name, VFS_NODE_DIR);
    if (!node) return -1;
    vfs_node_link_child(parent, node);
    return 0;
}

int vfs_rmdir(const char *path) {
    char name[MAX_NAME];
    vfs_node_t *parent = vfs_resolve_parent(path, name);
    if (!parent) return -1;

    vfs_node_t *target = vfs_node_find_child(parent, name);
    if (target && target->children) return -1;

    if (parent->ops && parent->ops->rmdir) {
        int ret = parent->ops->rmdir(parent, name);
        if (ret == 0 && target) {
            vfs_node_unlink_child(parent, target);
            kfree(target);
        }
        return ret;
    }

    if (!target) return -1;
    if (target->type != VFS_NODE_DIR) return -1;
    vfs_node_unlink_child(parent, target);
    kfree(target);
    return 0;
}

int vfs_unlink(const char *path) {
    char name[MAX_NAME];
    vfs_node_t *parent = vfs_resolve_parent(path, name);
    if (!parent) return -1;

    vfs_node_t *target = vfs_node_find_child(parent, name);

    if (parent->ops && parent->ops->unlink) {
        int ret = parent->ops->unlink(parent, name);
        if (ret == 0 && target) {
            vfs_node_unlink_child(parent, target);
            if (target->ref_count == 0) kfree(target);
        }
        return ret;
    }

    if (!target) return -1;
    if (target->ref_count > 0) return -1;
    vfs_node_unlink_child(parent, target);
    kfree(target);
    return 0;
}

int vfs_readdir(const char *path, uint32_t index, vfs_dirent_t *out) {
    if (!out) return -1;
    vfs_node_t *node = vfs_resolve_path(path);
    if (!node) return -1;

    if (node->type == VFS_NODE_MOUNTPOINT) {
        vfs_node_t *inner = (vfs_node_t *)node->priv;
        if (inner) node = inner;
    }

    if (node->ops && node->ops->readdir) {
        return node->ops->readdir(node, index, out);
    }

    uint32_t i = 0;
    for (vfs_node_t *c = node->children; c; c = c->next) {
        if (i == index) {
            size_t nlen = strlen(c->name);
            if (nlen >= MAX_NAME) nlen = MAX_NAME - 1;
            memcpy(out->name, c->name, nlen);
            out->name[nlen] = '\0';
            out->type = c->type;
            return 0;
        }
        i++;
    }
    return -1;
}

int vfs_create(const char *path, uint32_t type) {
    char name[MAX_NAME];
    vfs_node_t *parent = vfs_resolve_parent(path, name);
    if (!parent) return -1;

    if (parent->type == VFS_NODE_MOUNTPOINT) {
        vfs_node_t *inner = (vfs_node_t *)parent->priv;
        if (inner) parent = inner;
    }

    if (parent->ops && parent->ops->create) {
        return parent->ops->create(parent, name, type);
    }

    vfs_node_t *node = vfs_node_alloc(name, type);
    if (!node) return -1;
    vfs_node_link_child(parent, node);
    return 0;
}

vfs_node_t *vfs_register_node(const char *path, uint32_t type, vfs_node_ops_t *ops, void *priv) {
    char name[MAX_NAME];
    vfs_node_t *parent = vfs_resolve_parent(path, name);
    if (!parent) return NULL;

    vfs_node_t *node = vfs_node_alloc(name, type);
    if (!node) return NULL;

    node->ops  = ops;
    node->priv = priv;
    vfs_node_link_child(parent, node);
    return node;
}

static void vfs_setup_stdio(void) {
    vfs_node_t *stdin_node  = vfs_resolve_path("/dev/stdin");
    vfs_node_t *stdout_node = vfs_resolve_path("/dev/stdout");
    vfs_node_t *stderr_node = vfs_resolve_path("/dev/stderr");

    if (stdin_node) {
        fd_table[VFS_STDIN].node   = stdin_node;
        fd_table[VFS_STDIN].offset = 0;
        fd_table[VFS_STDIN].flags  = VFS_O_RDONLY;
        fd_table[VFS_STDIN].open   = 1;
        stdin_node->ref_count++;
    }

    if (stdout_node) {
        fd_table[VFS_STDOUT].node   = stdout_node;
        fd_table[VFS_STDOUT].offset = 0;
        fd_table[VFS_STDOUT].flags  = VFS_O_WRONLY;
        fd_table[VFS_STDOUT].open   = 1;
        stdout_node->ref_count++;
    }

    if (stderr_node) {
        fd_table[VFS_STDERR].node   = stderr_node;
        fd_table[VFS_STDERR].offset = 0;
        fd_table[VFS_STDERR].flags  = VFS_O_WRONLY;
        fd_table[VFS_STDERR].open   = 1;
        stderr_node->ref_count++;
    }
}

uint8_t vfs_init(void) {
    if (vfs_ready) return 0;

    vfs_root = vfs_node_alloc("/", VFS_NODE_DIR);
    if (!vfs_root) return VFS_ERR_NO_SLOTS;

    fd_table_init();

    mount_count = 0;
    vfs_ready   = 1;

    devfs_init();

    char letter = 'C';
    uint8_t ctrl_count = ahci_get_controller_count();
    for (uint8_t c = 0; c < ctrl_count; c++) {
        ahci_controller_t *ctrl = ahci_get_controller(c);
        if (!ctrl) continue;
        for (uint8_t p = 0; p < AHCI_MAX_PORTS; p++) {
            if (!ctrl->ports[p].present) continue;
            if (letter > 'Z') return VFS_ERR_NO_SLOTS;
            vfs_dev_addr_t addr = { .ahci = { .controller = c, .port = p } };
            uint8_t err = vfs_mount(AHCI, letter, addr);
            if (err == VFS_ERR_NO_SLOTS) return VFS_ERR_NO_SLOTS;
            letter++;
        }
    }

    uint8_t nvme_count = nvme_get_controller_count();
    for (uint8_t c = 0; c < nvme_count; c++) {
        nvme_controller_t *ctrl = nvme_get_controller(c);
        if (!ctrl) continue;
        for (uint32_t ns = 1; ns <= nvme_get_namespace_count(ctrl); ns++) {
            if (!nvme_namespace_present(ctrl, ns)) continue;
            if (letter > 'Z') return VFS_ERR_NO_SLOTS;
            vfs_dev_addr_t addr = { .nvme = { .controller = c, .nsid = ns } };
            uint8_t err = vfs_mount(NVME, letter, addr);
            if (err == VFS_ERR_NO_SLOTS) return VFS_ERR_NO_SLOTS;
            letter++;
        }
    }

    for (uint8_t bus = 0; bus <= 1; bus++) {
        for (uint8_t drive = 0; drive <= 1; drive++) {
            if (!ide_drive_present(bus, drive)) continue;
            if (letter > 'Z') return VFS_ERR_NO_SLOTS;
            vfs_dev_addr_t addr = { .ide = { .bus = bus, .drive = drive } };
            uint8_t err = vfs_mount(IDE, letter, addr);
            if (err == VFS_ERR_NO_SLOTS) return VFS_ERR_NO_SLOTS;
            letter++;
        }
    }

    vfs_setup_stdio();
    return VFS_OK;
}

vfs_node_t *vfs_get_root(void) {
    return vfs_root;
}

int vfs_dup(int fd) {
    if (fd < 0 || fd >= MAX_FD || !fd_table[fd].open) return -1;
    int new_fd = fd_alloc();
    if (new_fd < 0) return -1;
    fd_table[new_fd] = fd_table[fd];
    fd_table[new_fd].node->ref_count++;
    return new_fd;
}

int vfs_dup2(int old_fd, int new_fd) {
    if (old_fd < 0 || old_fd >= MAX_FD || !fd_table[old_fd].open) return -1;
    if (new_fd < 0 || new_fd >= MAX_FD) return -1;
    if (fd_table[new_fd].open) vfs_close(new_fd);
    fd_table[new_fd] = fd_table[old_fd];
    fd_table[new_fd].node->ref_count++;
    return new_fd;
}

int32_t vfs_filesize(int fd) {
    if (fd < 0 || fd >= MAX_FD || !fd_table[fd].open)
        return -1;

    vfs_node_t *node = fd_table[fd].node;
    if (!node)
        return -1;

    return (int32_t)node->size;
}

int32_t vfs_tell(int fd) {
    if (fd < 0 || fd >= MAX_FD || !fd_table[fd].open) return -1;
    return (int32_t)fd_table[fd].offset;
}

vfs_node_t *vfs_node_alloc_pub(const char *name, uint32_t type) {
    return vfs_node_alloc(name, type);
}

void vfs_node_link_child_pub(vfs_node_t *parent, vfs_node_t *child) {
    vfs_node_link_child(parent, child);
}

void vfs_node_unlink_child_pub(vfs_node_t *parent, vfs_node_t *child) {
    vfs_node_unlink_child(parent, child);
}

vfs_node_t *vfs_node_find_child_pub(vfs_node_t *parent, const char *name) {
    return vfs_node_find_child(parent, name);
}