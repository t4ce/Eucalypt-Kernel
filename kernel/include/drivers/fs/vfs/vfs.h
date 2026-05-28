#pragma once
#include <stdint.h>

#define VFS_OK                 0
#define VFS_ERR_LETTER_IN_USE  1
#define VFS_ERR_NO_SLOTS       2
#define VFS_ERR_INVALID_DEV    3
#define VFS_ERR_ALREADY_MOUNTED 4
#define VFS_ERR_FS_INIT        5
#define VFS_ERR_UNKNOWN_DEV    6

#define VFS_NODE_FILE          0
#define VFS_NODE_DIR           1
#define VFS_NODE_DEV           2
#define VFS_NODE_MOUNTPOINT    3
#define VFS_NODE_SYMLINK       4
#define VFS_NODE_PIPE          5

#define VFS_O_RDONLY           0x0000
#define VFS_O_WRONLY           0x0001
#define VFS_O_RDWR             0x0002
#define VFS_O_CREAT            0x0040
#define VFS_O_TRUNC            0x0200
#define VFS_O_APPEND           0x0400

#define VFS_SEEK_SET           0
#define VFS_SEEK_CUR           1
#define VFS_SEEK_END           2

#define VFS_STDIN              0
#define VFS_STDOUT             1
#define VFS_STDERR             2

#define MAX_NAME_LEN           256

typedef enum {
    fat12, fat16, fat32, exfat
} fs_t;

typedef enum {
    AHCI, NVME, IDE
} StorageDevType;

typedef struct vfs_blockdev {
    uint8_t  (*read)(struct vfs_blockdev *dev, uint32_t lba, uint8_t count, void *buf);
    uint8_t  (*write)(struct vfs_blockdev *dev, uint32_t lba, uint8_t count, const void *buf);
    void     *priv;
} vfs_blockdev_t;

typedef struct {
    union {
        struct { uint8_t controller; uint8_t port;  } ahci;
        struct { uint8_t controller; uint32_t nsid; } nvme;
        struct { uint8_t bus;        uint8_t drive; } ide;
    };
} vfs_dev_addr_t;

typedef struct vfs_dirent {
    char     name[MAX_NAME_LEN];
    uint32_t type;
} vfs_dirent_t;

typedef struct {
    struct vfs_node *(*lookup)(struct vfs_node *dir, const char *name);
    int32_t          (*read)(struct vfs_node *node, uint32_t offset, uint32_t size, uint8_t *buf);
    int32_t          (*write)(struct vfs_node *node, uint32_t offset, uint32_t size, const uint8_t *buf);
    int              (*readdir)(struct vfs_node *dir, uint32_t index, vfs_dirent_t *out);
    int              (*create)(struct vfs_node *dir, const char *name, uint32_t type);
    int              (*unlink)(struct vfs_node *dir, const char *name);
    int              (*rmdir)(struct vfs_node *dir, const char *name);
} vfs_node_ops_t;

typedef struct vfs_node {
    char             name[MAX_NAME_LEN];
    uint32_t         type;
    uint32_t         flags;
    uint32_t         size;
    uint32_t         ref_count;
    vfs_node_ops_t  *ops;
    void            *priv;
    struct vfs_node *parent;
    struct vfs_node *children;
    struct vfs_node *next;
} vfs_node_t;

typedef struct {
    char     name[MAX_NAME_LEN];
    uint32_t type;
    uint32_t size;
    uint32_t flags;
} vfs_stat_t;

typedef struct {
    vfs_node_t *node;
    uint32_t    offset;
    uint32_t    flags;
    uint8_t     open;
} vfs_file_t;

typedef struct {
    char           letter;
    vfs_blockdev_t blockdev;
    StorageDevType dev;
    vfs_dev_addr_t addr;
    void          *priv;
} vfs_mount_t;

typedef struct {
    vfs_dev_addr_t addr;
} vfs_blockdev_priv_t;

typedef struct {
    void    *vol;
    uint16_t start_cluster;
    uint32_t size;
    uint16_t dir_cluster;
} vfs_fat16_priv_t;

vfs_node_t *vfs_node_alloc(const char *name, uint32_t type);
void vfs_node_link_child(vfs_node_t *parent, vfs_node_t *child);
void vfs_node_unlink_child(vfs_node_t *parent, vfs_node_t *child);
vfs_node_t *vfs_node_find_child(vfs_node_t *parent, const char *name);
vfs_node_t *vfs_resolve_path(const char *path);
vfs_node_t *vfs_resolve_parent(const char *path, char *name_out);

uint8_t     vfs_init(void);
uint8_t     vfs_mount(StorageDevType dev, char letter, vfs_dev_addr_t addr);
void        vfs_unmount(char letter);
vfs_mount_t *vfs_get_mount(char letter);
vfs_node_t  *vfs_get_root(void);

int     vfs_open(const char *path, uint32_t flags);
int     vfs_close(int fd);
int32_t vfs_read(int fd, void *buf, uint32_t count);
int32_t vfs_write(int fd, const void *buf, uint32_t count);
int32_t vfs_seek(int fd, int32_t offset, int whence);
int32_t vfs_tell(int fd);
int     vfs_dup(int fd);
int     vfs_dup2(int old_fd, int new_fd);
int32_t vfs_filesize(int fd);

int vfs_stat(const char *path, vfs_stat_t *st);
int vfs_fstat(int fd, vfs_stat_t *st);
int vfs_mkdir(const char *path);
int vfs_rmdir(const char *path);
int vfs_unlink(const char *path);
int vfs_create(const char *path, uint32_t type);
int vfs_readdir(const char *path, uint32_t index, vfs_dirent_t *out);

vfs_node_t *vfs_register_node(const char *path, uint32_t type, vfs_node_ops_t *ops, void *priv);

fs_t        vfs_get_type(vfs_blockdev_t *blockdev);

vfs_node_t *vfs_node_alloc_pub(const char *name, uint32_t type);
void        vfs_node_link_child_pub(vfs_node_t *parent, vfs_node_t *child);
void        vfs_node_unlink_child_pub(vfs_node_t *parent, vfs_node_t *child);
vfs_node_t *vfs_node_find_child_pub(vfs_node_t *parent, const char *name);

uint8_t fat16_create_dirent_update(const void *vol_ptr, uint16_t dir_cluster,
                                   const char *name, uint16_t cluster, uint32_t size);