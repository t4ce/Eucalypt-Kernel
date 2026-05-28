#pragma once

#include <stdint.h>
#include <drivers/fs/vfs/vfs.h>

extern void    *ramfs_addr;
extern uint64_t ramfs_size;

typedef struct {
    char *name;
    char *data;
    int   size;
} ramfs_file_t;

void          ramfs_init    (void);
ramfs_file_t *ramfs_read    (char *archive, char *filename);
int           ramfs_list    (char *archive, char *directory, char **filenames, int max_files);
uint8_t       ramfs_mount   (void *archive, uint64_t archive_size, uint64_t capacity);
void          ramfs_unmount (void);