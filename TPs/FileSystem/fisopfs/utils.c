#include "utils.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

const char *
get_filename_from_path(const char *path)
{
	if (!path || *path == '\0')
		return NULL;

	const char *last_slash = strrchr(path, '/');
	if (!last_slash)
		return path;

	return last_slash + 1;
}

void
get_parent_path_from_path(char *parent_path)
{
	char *last_slash = strrchr(parent_path, '/');

	if (last_slash != NULL)
		*last_slash = '\0';
	else
		parent_path[0] = '\0';
}

int
next_free_inode_index(const char *path)
{
	for (int i = 0; i < MAX_INODES; i++) {
		if (strcmp(superblock.inodes[i].path, path) == 0)
			THROW(EEXIST)
		if (superblock.inodes_states[i] == FREE)
			return i;
	}

	THROW(ENOSPC);
}

int
new_inode(const char *path, mode_t mode, InodeType type)
{
	if (strlen(path) - 1 > MAX_DATA_SIZE)
		THROW(ENAMETOOLONG)

	const char *filename = get_filename_from_path(path);
	if (!filename)
		return -1;

	int free_inode_index = next_free_inode_index(filename);
	if (free_inode_index < 0)
		return free_inode_index;

	Inode new_inode = { .type = type,
		            .mode = mode,
		            .size = 0,
		            .uid = getuid(),
		            .gid = getgid(),
		            .last_access = time(NULL),
		            .last_modification = time(NULL),
		            .creation_time = time(NULL),
		            .content = { 0 } };

	strcpy(new_inode.path, filename);

	if (type == REGULAR) {
		char parent_path[MAX_PATH_LENGTH];
		memcpy(parent_path, path + 1, strlen(path) - 1);
		parent_path[strlen(path) - 1] = '\0';

		get_parent_path_from_path(parent_path);

		if (strlen(parent_path) == 0)
			strcpy(parent_path, ROOT_PATH);

		strcpy(new_inode.directory_path, parent_path);

	} else {
		strcpy(new_inode.directory_path, ROOT_PATH);
	}

	superblock.inodes[free_inode_index] = new_inode;
	superblock.inodes_states[free_inode_index] = OCCUPIED;

	return 0;
}

void
build_root_directory()
{
	Inode *root_dir = superblock.inodes;
	root_dir->type = DIRECTORY;
	root_dir->mode = __S_IFDIR | 0755;
	root_dir->size = MAX_DIRECTORY_SIZE;
	root_dir->uid = 1717;
	root_dir->gid = getgid();
	root_dir->last_access = time(NULL);
	root_dir->last_modification = time(NULL);
	root_dir->creation_time = time(NULL);
	strcpy(root_dir->path, ROOT_PATH);
	memset(root_dir->content, 0, sizeof(root_dir->content));
	strcpy(root_dir->directory_path, "");
	superblock.inodes_states[0] = OCCUPIED;
}

void
build_superblock()
{
	memset(superblock.inodes, 0, sizeof(superblock.inodes));
	memset(superblock.inodes_states, 0, sizeof(superblock.inodes_states));
	build_root_directory();
}

int
get_inode_index(const char *path)
{
	if (!path || strcmp(path, "") == 0)
		return -1;

	if (strcmp(path, ROOT_PATH) == 0)
		return 0;

	const char *filename = get_filename_from_path(path);
	if (!filename)
		return -1;

	for (int i = 0; i < MAX_INODES; i++) {
		if (superblock.inodes_states[i] == 1) {
			if (strcmp(filename, superblock.inodes[i].path) == 0)
				return i;
		}
	}

	return -1;
}

int
is_child_inode(const Inode *dir_inode, const Inode *child_inode)
{
	return strcmp(child_inode->directory_path, dir_inode->path) == 0;
}

Inode **
get_files_in_directory(const char *path_dir, int *nfiles)
{
	int max = 0;
	Inode **files = malloc(MAX_INODES * sizeof(Inode *));
	const char *filename = get_filename_from_path(path_dir);

	for (int i = 0; i < MAX_INODES; i++) {
		if (strcmp(superblock.inodes[i].directory_path, filename) == 0)
			files[max++] = superblock.inodes + i;
	}

	*nfiles = max;
	return files;
}