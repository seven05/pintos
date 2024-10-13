/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"

static bool file_backed_swap_in (struct page *page, void *kva);
static bool file_backed_swap_out (struct page *page);
static void file_backed_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations file_ops = {
	.swap_in = file_backed_swap_in,
	.swap_out = file_backed_swap_out,
	.destroy = file_backed_destroy,
	.type = VM_FILE,
};

/* The initializer of file vm */
void
vm_file_init (void) {
	printf("\n------- vm_file_init -------");
	printf("\n------- vm_file_init end -------");
}

/* Initialize the file backed page */
bool
file_backed_initializer (struct page *page, enum vm_type type, void *kva) {
	printf("\n------- file_backed_initializer -------");
	/* Set up the handler */
	page->operations = &file_ops;

	struct file_page *file_page = &page->file;
	printf("\n------- file_backed_initializer end -------");
}

/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in (struct page *page, void *kva) {
	printf("\n------- file_backed_swap_in -------");
	struct file_page *file_page UNUSED = &page->file;
	printf("\n------- file_backed_swap_in end -------");
}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out (struct page *page) {
	printf("\n------- file_backed_swap_out -------");
	struct file_page *file_page UNUSED = &page->file;
	printf("\n------- file_backed_swap_out end -------");
}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy (struct page *page) {
	printf("\n------- file_backed_destroy -------");
	struct file_page *file_page UNUSED = &page->file;
	printf("\n------- file_backed_destroy end -------");
}

/* Do the mmap */
void *
do_mmap (void *addr, size_t length, int writable,
		struct file *file, off_t offset) {
	printf("\n------- do_mmap -------");
	printf("\n------- do_mmap end -------");
}

/* Do the munmap */
void
do_munmap (void *addr) {
	printf("\n------- do_munmap -------");
	printf("\n------- do_munmap end -------");
}
