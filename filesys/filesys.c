#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "devices/disk.h"

#include "filesys/fat.h"
/** #Project 4: File System and Soft Links */
#include "threads/thread.h"

/* The disk that contains the file system. */
struct disk *filesys_disk;

static void do_format (void);

/* Initializes the file system module.
 * If FORMAT is true, reformats the file system. */
void
filesys_init (bool format) {
	// /**/printf("\n------- filesys_init -------\n");
	filesys_disk = disk_get (0, 1);
	if (filesys_disk == NULL)
		PANIC ("hd0:1 (hdb) not present, file system initialization failed");

	inode_init ();

#ifdef EFILESYS
	fat_init ();

	if (format)
		do_format ();

	fat_open ();
#else
	/* Original FS */
	free_map_init ();

	if (format)
		do_format ();

	free_map_open ();
#endif
// /**/printf("------- filesys_init end -------\n\n");
}

/* Shuts down the file system module, writing any unwritten data
 * to disk. */
void
filesys_done (void) {
	// /**/printf("\n------- filesys_done -------\n");
	/* Original FS */
#ifdef EFILESYS
	fat_close ();
#else
	free_map_close ();
#endif
	// /**/printf("------- filesys_done end -------\n\n");
}

/* Creates a file named NAME with the given INITIAL_SIZE.
 * Returns true if successful, false otherwise.
 * Fails if a file named NAME already exists,
 * or if internal memory allocation fails. */
bool
filesys_create (const char *name, off_t initial_size) {
	// /**/printf("\n------- filesys_create -------\n");
	cluster_t inode_cluster = fat_create_chain(0);
	disk_sector_t inode_sector = cluster_to_sector(inode_cluster);
	struct dir *dir = dir_open_root ();
	bool success = (dir != NULL
			&& inode_create (inode_sector, initial_size, FILE_TYPE)
			&& dir_add (dir, name, inode_sector));
	if (!success && inode_sector != 0)
		fat_remove_chain (inode_cluster, 1);

	dir_close (dir);

	// /**/printf("------- filesys_create end -------\n\n");
	return success;
}

/* Opens the file with the given NAME.
 * Returns the new file if successful or a null pointer
 * otherwise.
 * Fails if no file named NAME exists,
 * or if an internal memory allocation fails. */
struct file *
filesys_open (const char *name) {
	// /**/printf("\n------- filesys_open -------\n");
	struct dir *dir = dir_open_root ();
	struct inode *inode = NULL;

	if (dir != NULL)
		dir_lookup (dir, name, &inode);
	dir_close (dir);

	// /**/printf("------- filesys_open end -------\n\n");
	return file_open (inode);
}

/* Deletes the file named NAME.
 * Returns true if successful, false on failure.
 * Fails if no file named NAME exists,
 * or if an internal memory allocation fails. */
bool
filesys_remove (const char *name) {
	// /**/printf("\n------- filesys_remove -------\n");
	struct dir *dir = dir_open_root ();
	bool success = dir != NULL && dir_remove (dir, name);
	dir_close (dir);

	// /**/printf("------- filesys_remove end -------\n\n");
	return success;
}

/* Formats the file system. */
static void
do_format (void) {
	// /**/printf("\n------- do_format -------\n");
	printf ("Formatting file system...");

#ifdef EFILESYS
	/* Create FAT and save it to the disk. */
	fat_create ();

	/* Root Directory 생성 */
	disk_sector_t root = cluster_to_sector(ROOT_DIR_CLUSTER);
	if (!dir_create(root, 16))
		PANIC("root directory creation failed");
	fat_close ();
#else
	free_map_create ();
	if (!dir_create (ROOT_DIR_SECTOR, 16))
		PANIC ("root directory creation failed");
	free_map_close ();
#endif

	printf ("done.\n");
	// /**/printf("------- do_format end -------\n\n");
}

bool filesys_chdir(const char *dir_name) {
	bool success = false;
	return success;
}

bool filesys_mkdir(const char *dir_name) {
	bool success = false;
	return success;
}