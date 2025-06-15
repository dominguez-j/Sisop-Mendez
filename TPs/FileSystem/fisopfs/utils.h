#ifndef FISOPS_UTILS_H
#define FISOPS_UTILS_H

#include "inode.h"
#include "superblock.h"

#define MAX_DIRECTORY_SIZE 1024
#define ROOT_PATH "/"

#define THROW(errnumber)                                                       \
	do {                                                                   \
		errno = errnumber;                                             \
		return -errnumber;                                             \
	} while (0);

const char *get_filename_from_path(const char *path);
void get_parent_path_from_path(char *parent_path);
int next_free_inode_index(const char *path);
int new_inode(const char *path, mode_t mode, InodeType type);
void build_root_directory();
void build_superblock();
int get_inode_index(const char *path);
int is_child_inode(const Inode *dir_inode, const Inode *child_inode);
Inode **get_files_in_directory(const char *path_dir, int *nfiles);

#endif  // FISOPS_UTILS_H