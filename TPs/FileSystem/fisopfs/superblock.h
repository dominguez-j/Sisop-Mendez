#ifndef FISOPFS_SUPERBLOCK_H
#define FISOPFS_SUPERBLOCK_H

#include "inode.h"

#define MAX_INODES 1024

typedef enum { FREE, OCCUPIED } InodeState;

typedef struct {
	Inode inodes[MAX_INODES];
	InodeState inodes_states[MAX_INODES];
} Superblock;

extern Superblock superblock;

#endif  // FISOPFS_SUPERBLOCK_H