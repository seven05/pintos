#include "filesys/fat.h"
#include "devices/disk.h"
#include "filesys/filesys.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include <stdio.h>
#include <string.h>

/* Should be less than DISK_SECTOR_SIZE */
struct fat_boot {
	unsigned int magic;
	unsigned int sectors_per_cluster; /* Fixed to 1 */
	unsigned int total_sectors;
	unsigned int fat_start;
	unsigned int fat_sectors; /* Size of FAT in sectors. */
	unsigned int root_dir_cluster;
};

/* FAT FS */
struct fat_fs {
	struct fat_boot bs;
	unsigned int *fat;			// FAT table
	unsigned int fat_length;
	disk_sector_t data_start;
	cluster_t last_clst;
	struct lock write_lock;
};

static struct fat_fs *fat_fs;

void fat_boot_create (void);
void fat_fs_init (void);

void
fat_init (void) {
	// /**/printf("\n------- fat_init -------\n");
	fat_fs = calloc (1, sizeof (struct fat_fs));
	if (fat_fs == NULL)
		PANIC ("FAT init failed");

	// Read boot sector from the disk
	unsigned int *bounce = malloc (DISK_SECTOR_SIZE);
	if (bounce == NULL)
		PANIC ("FAT init failed");
	disk_read (filesys_disk, FAT_BOOT_SECTOR, bounce);
	memcpy (&fat_fs->bs, bounce, sizeof (fat_fs->bs));
	free (bounce);

	// Extract FAT info
	if (fat_fs->bs.magic != FAT_MAGIC)
		fat_boot_create ();
	fat_fs_init ();
	// /**/printf("------- fat_init end -------\n\n");
}

void
fat_open (void) {
	// /**/printf("\n------- fat_open -------\n");
	fat_fs->fat = calloc (fat_fs->fat_length, sizeof (cluster_t));
	if (fat_fs->fat == NULL)
		PANIC ("FAT load failed");

	// Load FAT directly from the disk
	uint8_t *buffer = (uint8_t *) fat_fs->fat;
	off_t bytes_read = 0;
	off_t bytes_left = sizeof (fat_fs->fat);
	const off_t fat_size_in_bytes = fat_fs->fat_length * sizeof (cluster_t);
	for (unsigned i = 0; i < fat_fs->bs.fat_sectors; i++) {
		bytes_left = fat_size_in_bytes - bytes_read;
		if (bytes_left >= DISK_SECTOR_SIZE) {
			disk_read (filesys_disk, fat_fs->bs.fat_start + i,
			           buffer + bytes_read);
			bytes_read += DISK_SECTOR_SIZE;
		} else {
			uint8_t *bounce = malloc (DISK_SECTOR_SIZE);
			if (bounce == NULL)
				PANIC ("FAT load failed");
			disk_read (filesys_disk, fat_fs->bs.fat_start + i, bounce);
			memcpy (buffer + bytes_read, bounce, bytes_left);
			bytes_read += bytes_left;
			free (bounce);
		}
	}
	// /**/printf("------- fat_open end -------\n\n");
}

void
fat_close (void) {
	// Write FAT boot sector
	// /**/printf("\n------- fat_close -------\n");
	uint8_t *bounce = calloc (1, DISK_SECTOR_SIZE);
	if (bounce == NULL)
		PANIC ("FAT close failed");
	memcpy (bounce, &fat_fs->bs, sizeof (fat_fs->bs));
	disk_write (filesys_disk, FAT_BOOT_SECTOR, bounce);
	free (bounce);

	// Write FAT directly to the disk
	uint8_t *buffer = (uint8_t *) fat_fs->fat;
	off_t bytes_wrote = 0;
	off_t bytes_left = sizeof (fat_fs->fat);
	const off_t fat_size_in_bytes = fat_fs->fat_length * sizeof (cluster_t);
	for (unsigned i = 0; i < fat_fs->bs.fat_sectors; i++) {
		bytes_left = fat_size_in_bytes - bytes_wrote;
		if (bytes_left >= DISK_SECTOR_SIZE) {
			disk_write (filesys_disk, fat_fs->bs.fat_start + i,
			            buffer + bytes_wrote);
			bytes_wrote += DISK_SECTOR_SIZE;
		} else {
			bounce = calloc (1, DISK_SECTOR_SIZE);
			if (bounce == NULL)
				PANIC ("FAT close failed");
			memcpy (bounce, buffer + bytes_wrote, bytes_left);
			disk_write (filesys_disk, fat_fs->bs.fat_start + i, bounce);
			bytes_wrote += bytes_left;
			free (bounce);
		}
	}
	// /**/printf("------- fat_close end -------\n\n");
}

void
fat_create (void) {
	// /**/printf("\n------- fat_createfat_create -------\n");
	// Create FAT boot
	fat_boot_create ();
	fat_fs_init ();

	// Create FAT table
	fat_fs->fat = calloc (fat_fs->fat_length, sizeof (cluster_t));
	if (fat_fs->fat == NULL)
		PANIC ("FAT creation failed");

	// Set up ROOT_DIR_CLST
	fat_put (ROOT_DIR_CLUSTER, EOChain);

	// Fill up ROOT_DIR_CLUSTER region with 0
	uint8_t *buf = calloc (1, DISK_SECTOR_SIZE);
	if (buf == NULL)
		PANIC ("FAT create failed due to OOM");
	disk_write (filesys_disk, cluster_to_sector (ROOT_DIR_CLUSTER), buf);
	free (buf);
	// /**/printf("------- fat_createfat_create end -------\n\n");
}

void
fat_boot_create (void) {
	// /**/printf("\n------- fat_boot_create -------\n");
	unsigned int fat_sectors =
	    (disk_size (filesys_disk) - 1)
	    / (DISK_SECTOR_SIZE / sizeof (cluster_t) * SECTORS_PER_CLUSTER + 1) + 1;
	fat_fs->bs = (struct fat_boot){
	    .magic = FAT_MAGIC,
	    .sectors_per_cluster = SECTORS_PER_CLUSTER,
	    .total_sectors = disk_size (filesys_disk),
	    .fat_start = 1,
	    .fat_sectors = fat_sectors,
	    .root_dir_cluster = ROOT_DIR_CLUSTER,
	};
	// /**/printf("------- fat_boot_create end -------\n\n");
}

void
fat_fs_init (void) {
	/* TODO: Your code goes here. */
	// /**/printf("\n------- fat_fs_init -------\n");
	fat_fs->fat_length = disk_size(filesys_disk) - fat_fs->bs.fat_sectors - BOOT_SECTOR_SIZE;	// 데이터블럭 사이즈
	fat_fs->data_start = (disk_sector_t)(fat_fs->bs.fat_start + fat_fs->bs.fat_sectors);
	// /**/printf("fat_fs->fat_length : %d\n", fat_fs->fat_length);
	// /**/printf("fat_fs->data_start : %d\n", fat_fs->data_start);
	// /**/printf("fat_fs->bs.fat_sectors : %d\n", fat_fs->bs.fat_sectors);
	// /**/printf("------- fat_fs_init end -------\n\n");
}

/*----------------------------------------------------------------------------*/
/* FAT handling                                                               */
/*----------------------------------------------------------------------------*/

/* Add a cluster to the chain.
 * If CLST is 0, start a new chain.
 * Returns 0 if fails to allocate a new cluster. */
cluster_t
fat_create_chain (cluster_t clst) {
	// TODO: Your code goes here. */
    //? 1번은 root cluster
	// /**/printf("\n------- fat_create_chain -------\n");
	// /**/printf("clst : %d\n", clst);
	cluster_t i = 2;
	while (fat_get(i) != 0 && i < fat_fs->fat_length) {
		++i;
	}
	
    //! FAT가 꽉 찼다
	if (i == fat_fs->fat_length) {
		return 0;
	}
	
	fat_put(i, EOChain);

    //! 새로운 chain을 만든다
	if (clst == 0) {
		return i;
	}

	while(fat_get(clst) != EOChain) {
		clst = fat_get(clst);
	}
	// /**/print_fat_chain(134);

	fat_put(clst, i);
	// /**/printf("------- fat_create_chain end -------\n\n");
	return i;
}

/* Remove the chain of clusters starting from CLST.
 * If PCLST is 0, assume CLST as the start of the chain. */
void fat_remove_chain(cluster_t clst, cluster_t pclst) {
    /* TODO: Your code goes here. */
	// /**/printf("\n------- fat_remove_chain -------\n");
	// /**/printf("clst : %d\n", clst);
	// /**/printf("pclst : %d\n", pclst);
    cluster_t target = clst;

    while (fat_get(target) != EOChain) {  // 순회하면서 FAT에서 할당 해제
        cluster_t next = fat_get(target);
		// /**/printf("target : %d, next : %d\n", target, next);

        fat_put(target, 0);
        target = next;
    }

    if (!pclst)  // pcluster가 입력됬으면 pcluster를 chain으로 끝으로 만듬 */
        fat_put(pclst, EOChain);
	// /**/printf("------- fat_remove_chain end -------\n\n");
}

/* Update a value in the FAT table. */
void
fat_put (cluster_t clst, cluster_t val) {
	/* TODO: Your code goes here. */
	// printf("\n------- fat_put -------\n");
	fat_fs->fat[clst] = val;
	// printf("------- fat_put end -------\n\n");
}

/* Fetch a value in the FAT table. */
cluster_t
fat_get (cluster_t clst) {
	/* TODO: Your code goes here. */
	// printf("\n------- fat_get -------\n");
	// printf("------- fat_get end -------\n\n");
	return fat_fs->fat[clst];
}

/* Covert a cluster # to a sector number. */
disk_sector_t
cluster_to_sector (cluster_t clst) {
	/* TODO: Your code goes here. */
	// /**/printf("\n------- cluster_to_sector -------\n");
	// /**/printf("------- cluster_to_sector end -------\n\n");
	return fat_fs->data_start + clst /* -1 */;
}

/** Project 4: Indexed and Extensible Files - 섹터 넘버를 clst로 변환해서 리턴 */
cluster_t sector_to_cluster(disk_sector_t sctr) {
	cluster_t clst = sctr - fat_fs->data_start;
	return clst < 2 ? 0 : clst;
}

// FAT 체인을 출력하는 함수
void print_fat_chain(cluster_t start_clst) {
    cluster_t current_clst = start_clst;

    if (current_clst == 0) {
        printf("Invalid start cluster.\n");
        return;
    }

    printf("FAT chain starting at cluster %u:\n", current_clst);

    // 클러스터 체인을 EOF가 나올 때까지 따라감
    while (current_clst != EOChain) {
        printf("%u -> ", current_clst);  // 현재 클러스터 출력

        // 다음 클러스터를 FAT 테이블에서 가져옴
        current_clst = fat_get(current_clst);

        // 다음 클러스터가 EOF인지 확인
        if (current_clst == EOChain) {
            printf("EOF\n");
            break;
        }

        // 만약 에러가 있거나 잘못된 값일 경우
        if (current_clst == 0) {
            printf("Error: Invalid cluster in chain.\n");
            break;
        }
    }
}