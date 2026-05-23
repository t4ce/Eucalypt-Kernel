#pragma once
#include <stdint.h>

#define MAX_NAME_LEN 256

typedef struct devfs_dev {
    char    name[MAX_NAME_LEN];
    int32_t (*read)(struct devfs_dev *dev, void *buf, uint32_t count);
    int32_t (*write)(struct devfs_dev *dev, const void *buf, uint32_t count);
    void   *priv;
} devfs_dev_t;

void devfs_init(void);
int  devfs_register(const char *name,
                    int32_t (*read)(devfs_dev_t *, void *, uint32_t),
                    int32_t (*write)(devfs_dev_t *, const void *, uint32_t),
                    void *priv);
int          devfs_unregister(const char *name);
devfs_dev_t *devfs_get(const char *name);