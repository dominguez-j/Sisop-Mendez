#define FUSE_USE_VERSION 30

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <errno.h>

#include <fuse.h>

#include "utils.h"
#include "superblock.h"
#include "inode.h"

char filedisk[MAX_PATH_LENGTH] = "fs.fisopfs";

static int
fisopfs_getattr(const char *path, struct stat *st)
{
	printf("[debug] fisopfs_getattr - path: %s\n", path);

	int inode_index = get_inode_index(path);
	if (inode_index < 0)
		THROW(ENOENT)

	Inode inode = superblock.inodes[inode_index];
	st->st_dev = 0;
	st->st_ino = inode_index;
	st->st_uid = inode.uid;
	st->st_atime = inode.last_access;
	st->st_mtime = inode.last_modification;
	st->st_ctime = inode.creation_time;
	st->st_size = inode.size;
	st->st_gid = inode.gid;
	st->st_nlink = 2;
	st->st_mode = __S_IFDIR | 0755;
	if (inode.type == REGULAR) {
		st->st_mode = __S_IFREG | 0644;
		st->st_nlink = 1;
	}

	return 0;
}

static int
fisopfs_readdir(const char *path,
                void *buffer,
                fuse_fill_dir_t filler,
                off_t offset,
                struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_readdir(%s)\n", path);

	filler(buffer, ".", NULL, 0);
	filler(buffer, "..", NULL, 0);

	int inode_index = get_inode_index(path);
	if (inode_index < 0)
		THROW(ENOENT)

	Inode dir_inode = superblock.inodes[inode_index];

	if (dir_inode.type != DIRECTORY)
		THROW(ENOTDIR)

	dir_inode.last_access = time(NULL);

	for (int i = 1; i < MAX_INODES; i++) {
		if (superblock.inodes_states[i] == OCCUPIED &&
		    is_child_inode(&dir_inode, superblock.inodes + i)) {
			filler(buffer, superblock.inodes[i].path, NULL, 0);
		}
	}

	return 0;
}

static int
fisopfs_read(const char *path,
             char *buf,
             size_t size,
             off_t offset,
             struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_read - path: %s, offset: %lu, size: %lu\n",
	       path,
	       offset,
	       size);

	if (offset < 0 || size < 0)
		THROW(EINVAL)

	int inode_index = get_inode_index(path);
	if (inode_index < 0)
		THROW(ENOENT)

	Inode *inode = superblock.inodes + inode_index;
	if (inode->type == DIRECTORY)
		THROW(EISDIR)

	if (offset >= inode->size)
		return 0;

	size_t bytes_to_read = size;
	if (offset + size > inode->size) {
		bytes_to_read = inode->size - offset;
	}

	memcpy(buf, inode->content + offset, bytes_to_read);
	inode->last_access = time(NULL);

	return bytes_to_read;
}

static int
fisopfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_touch - path: %s\n", path);

	return new_inode(path, mode, REGULAR);
}

static int
fisopfs_mkdir(const char *path, mode_t mode)
{
	printf("[debug] fisopfs_mkdir - path: %s\n", path);

	return new_inode(path, mode, DIRECTORY);
}

static int
fisopfs_rmdir(const char *path)
{
	printf("[debug] fisopfs_rmdir - path: %s\n", path);

	int inode_index = get_inode_index(path);
	if (inode_index < 0)
		THROW(ENOENT)


	Inode *inode = superblock.inodes + inode_index;
	if (inode->type != DIRECTORY)
		THROW(ENOTDIR)

	int nfiles = 0;
	Inode **files = get_files_in_directory(path, &nfiles);

	if (!files)
		free(files);
	if (nfiles > 0)
		THROW(ENOTEMPTY)

	superblock.inodes_states[inode_index] = FREE;
	memset(inode, 0, sizeof(Inode));
	return 0;
}

static int
fisopfs_unlink(const char *path)
{
	printf("[debug] fisopfs_unlink(%s)\n", path);

	int inode_index = get_inode_index(path);
	if (inode_index < 0)
		THROW(ENOENT)

	Inode *inode = superblock.inodes + inode_index;
	if (inode->type != REGULAR)
		THROW(EISDIR)

	memset(inode, 0, sizeof(Inode));
	superblock.inodes_states[inode_index] = FREE;

	return 0;
}

void *
fisopfs_init(struct fuse_conn_info *conn)
{
	printf("[debug] fisop_init\n");

	FILE *file = fopen(filedisk, "r");
	if (!file) {
		build_superblock();
	} else {
		int i = fread(&superblock, sizeof(superblock), 1, file);
		if (i != 1)
			return NULL;
		fclose(file);
	}
	return 0;
}

int
fisopfs_write(const char *path,
              const char *data,
              size_t size_data,
              off_t offset,
              struct fuse_file_info *fuse_info)
{
	printf("[debug] fisops_write (%s) \n", path);

	if (offset + size_data > MAX_DATA_SIZE)
		THROW(EFBIG)

	int inode_index = get_inode_index(path);
	if (inode_index < 0) {
		int result = fisopfs_create(path, 33204, fuse_info);
		if (result < 0)
			return result;
		inode_index = get_inode_index(path);
	}

	if (inode_index < 0)
		THROW(ENOENT)

	Inode *inode = superblock.inodes + inode_index;
	if (inode->size < offset)
		THROW(EINVAL)
	if (inode->type != REGULAR)
		THROW(EACCES)

	memcpy(inode->content + offset, data, size_data);
	inode->last_access = time(NULL);
	inode->last_modification = time(NULL);
	inode->size = strlen(inode->content);
	inode->content[inode->size] = '\0';

	return (int) size_data;
}

void
fisopfs_destroy(void *private_data)
{
	printf("[debug] fisop_destroy\n");

	FILE *file = fopen(filedisk, "w");
	if (!file)
		return;

	size_t w = fwrite(&superblock, sizeof(superblock), 1, file);
	if (w != 1) {
		fclose(file);
		return;
	}

	fflush(file);
	fclose(file);
}

int
fisopfs_utimens(const char *path, const struct timespec tv[2])
{
	printf("[debug] fisop_utimens\n");

	int inode_index = get_inode_index(path);
	if (inode_index < 0)
		THROW(ENOENT)

	Inode *inode = superblock.inodes + inode_index;
	inode->last_access = tv[0].tv_sec;
	inode->last_modification = tv[1].tv_sec;
	return 0;
}

static int
fisopfs_truncate(const char *path, off_t size)
{
	printf("[debug] fisopfs_truncate - path: %s\n", path);

	if (size > MAX_DATA_SIZE)
		THROW(EINVAL)

	int inode_index = get_inode_index(path);
	if (inode_index < 0)
		THROW(ENOENT)

	Inode *inode = superblock.inodes + inode_index;
	if (inode->type != REGULAR)
		THROW(EISDIR)

	if (size == 0) {
		for (int i = 0; i < MAX_DATA_SIZE; i++)
			inode->content[i] = 0;
		inode->size = 0;
		inode->last_modification = time(NULL);
		return 0;
	}

	int content_size = strlen(inode->content) + 1;
	inode->size = size;
	int min_size = size < content_size ? size : content_size;
	for (int i = min_size; i < content_size; i++)
		inode->content[i] = 0;
	for (int i = content_size; i < size; i++)
		inode->content[i] = 0;

	if (content_size > size) {
		inode->content[size] = '\0';
	}

	inode->size = size;
	inode->last_modification = time(NULL);
	return 0;
}

static int
fisopfs_flush(const char *path, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_flush(%s)\n", path);
	fisopfs_destroy(NULL);
	return 0;
}

static struct fuse_operations operations = { .getattr = fisopfs_getattr,
	                                     .readdir = fisopfs_readdir,
	                                     .read = fisopfs_read,
	                                     .create = fisopfs_create,
	                                     .mkdir = fisopfs_mkdir,
	                                     .rmdir = fisopfs_rmdir,
	                                     .unlink = fisopfs_unlink,
	                                     .init = fisopfs_init,
	                                     .write = fisopfs_write,
	                                     .destroy = fisopfs_destroy,
	                                     .utimens = fisopfs_utimens,
	                                     .truncate = fisopfs_truncate,
	                                     .flush = fisopfs_flush };

int
main(int argc, char *argv[])
{
	for (int i = 1; i < argc - 1; i++) {
		if (strcmp(argv[i], "--filedisk") == 0) {
			strcpy(filedisk, argv[i + 1]);

			for (int j = i; j < argc - 1; j++) {
				argv[j] = argv[j + 2];
			}

			argc = argc - 2;
			break;
		}
	}
	return fuse_main(argc, argv, &operations, NULL);
}