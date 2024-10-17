/* anon.c: Implementation of page for non-disk image (a.k.a. anonymous page). */

#include "vm/vm.h"
#include "devices/disk.h"

#include "string.h"
#include "bitmap.h"
#include "threads/mmu.h"

/* DO NOT MODIFY BELOW LINE */
static struct disk *swap_disk;
static bool anon_swap_in (struct page *page, void *kva);
static bool anon_swap_out (struct page *page);
static void anon_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations anon_ops = {
	.swap_in = anon_swap_in,
	.swap_out = anon_swap_out,
	.destroy = anon_destroy,
	.type = VM_ANON,
};

struct bitmap *swap_table;
size_t swap_max;

/* Initialize the data for anonymous pages */
void
vm_anon_init (void) {
	// /**/printf("------- vm_anon_init -------\n");
	/* TODO: Set up the swap_disk. */
	swap_disk = disk_get(1, 1);
	swap_max = disk_size(swap_disk) / SWAP_SIZE;
	swap_table = bitmap_create(swap_max);
	// /**/printf("------- vm_anon_init end -------\n");
}

/* Initialize the file mapping */
bool
anon_initializer (struct page *page, enum vm_type type, void *kva) {
	// /**/printf("------- anon_initializer -------\n");
	/* Set up the handler */
	memset(&page->uninit, 0, sizeof(struct uninit_page));
	page->operations = &anon_ops;

	struct anon_page *anon_page = &page->anon;

	anon_page->slot = BITMAP_ERROR;
	// /**/printf("------- anon_initializer end -------\n");
	return true;
}

/* Swap in the page by read contents from the swap disk. */
static bool
anon_swap_in (struct page *page, void *kva) {
	// /**/printf("------- anon_swap_in -------\n");
	struct anon_page *anon_page = &page->anon;
	if (anon_page->slot == BITMAP_ERROR) {
		return false;
	}
	for (int i=0; i<SWAP_SIZE; i++) {
		disk_read(swap_disk, (anon_page->slot)*SWAP_SIZE+i, kva+DISK_SECTOR_SIZE*i);
	}
	bitmap_set(swap_table, anon_page->slot, false);
	anon_page->slot = BITMAP_ERROR;
	return true;
	// /**/printf("------- anon_swap_in end -------\n");
}

/* Swap out the page by writing contents to the swap disk. */
static bool
anon_swap_out (struct page *page) {
	// /**/printf("------- anon_swap_out -------\n");
	struct anon_page *anon_page = &page->anon;
	size_t swap_idx = bitmap_scan_and_flip (swap_table, 0, 1, false);
	if (swap_idx == BITMAP_ERROR) {
		return false;
	}
	for (int i=0; i<SWAP_SIZE; i++) {
		disk_write(swap_disk, swap_idx*SWAP_SIZE+i, (page->va)+DISK_SECTOR_SIZE*i);
	}
	anon_page->slot = swap_idx;
	page->frame->page = NULL;
	page->frame = NULL;

	pml4_clear_page(thread_current()->pml4, page->va);

	return true;
	// /**/printf("------- anon_swap_out end -------\n");
}

/* Destroy the anonymous page. PAGE will be freed by the caller. */
static void
anon_destroy (struct page *page) {
	// /**/printf("------- anon_destroy -------\n");
	struct anon_page *anon_page = &page->anon;
	
	// mytodo : destroy코드 필요? (있든 없든 결과는 같음. <24.10.11 anonymous 작성중>)
    if (anon_page->slot != BITMAP_ERROR)
        bitmap_reset(swap_table, anon_page->slot);

	if (page->frame) {
		list_remove(&page->frame->elem);
		page->frame->page = NULL;
		free(page->frame);
		page->frame = NULL;
	}

	// /**/printf("------- anon_destroy end -------\n");
}
