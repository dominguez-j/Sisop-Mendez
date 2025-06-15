#ifndef FISOPFS_INODE_H
#define FISOPFS_INODE_H

#include <stdlib.h>
#include <time.h>
#include <sys/types.h>

#define MAX_DATA_SIZE 1024
#define MAX_PATH_LENGTH 256

typedef enum { REGULAR, DIRECTORY } InodeType;

typedef struct {
	InodeType type;
	mode_t mode;
	size_t size;
	uid_t uid;
	gid_t gid;
	time_t last_access;
	time_t last_modification;
	time_t creation_time;
	char path[MAX_PATH_LENGTH];
	char content[MAX_DATA_SIZE];
	char directory_path[MAX_PATH_LENGTH];
} Inode;

#endif  // FISOPFS_INODE_H