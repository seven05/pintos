/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "threads/vaddr.h"
#include "userprog/process.h"
#include "userprog/syscall.h"

#include "threads/mmu.h"

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
	// /**/printf("------- vm_file_init -------\n");
	// /**/printf("------- vm_file_init end -------\n");
}

/* Initialize the file backed page */
bool
file_backed_initializer (struct page *page, enum vm_type type, void *kva) {
	// /**/printf("------- file_backed_initializer -------\n");
	/* Set up the handler */
	page->operations = &file_ops;

	struct file_page *file_page = &page->file;

	struct container *container = (struct container *)page->uninit.aux;
	file_page->file = container->file;
	file_page->offset = container->offset;
	file_page->page_read_bytes = container->page_read_bytes;

	return true;
	// /**/printf("------- file_backed_initializer end -------\n");
}

/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in (struct page *page, void *kva) {
	// /**/printf("------- file_backed_swap_in -------\n");
	struct file_page *file_page UNUSED = &page->file;
	return lazy_load_segment(page, file_page);
	// /**/printf("------- file_backed_swap_in end -------\n");
}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out (struct page *page) {
	// /**/printf("------- file_backed_swap_out -------\n");
	struct file_page *file_page UNUSED = &page->file;
	if (pml4_is_dirty(thread_current()->pml4, page->va)) {
		file_write_at(file_page->file, page->va, file_page->page_read_bytes, file_page->offset);
		pml4_set_dirty(thread_current()->pml4, page->va, false);
	}

	page->frame->page = NULL;
	page->frame = NULL;
    pml4_clear_page(thread_current()->pml4, page->va);
	
	return true;
	// /**/printf("------- file_backed_swap_out end -------\n");
}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy (struct page *page) {
	// /**/printf("------- file_backed_destroy -------\n");
	struct file_page *file_page UNUSED = &page->file;

	if (pml4_is_dirty(thread_current()->pml4, page->va)) {
		file_write_at(file_page->file, page->va, file_page->page_read_bytes, file_page->offset);
		pml4_set_dirty(thread_current()->pml4, page->va, false);
	}

	if (page->frame) {
		list_remove(&page->frame->elem);
		page->frame->page = NULL;
		page->frame = NULL;
		// palloc_free_page(page->frame->kva);
		free(page->frame);
	}

	pml4_clear_page(thread_current()->pml4, page->va);
	// free(page);
	// /**/printf("------- file_backed_destroy end -------\n");
}

/* Do the mmap */
void *
do_mmap (void *addr, size_t length, int writable,
		struct file *file, off_t offset) {
	// /**/printf("------- do_mmap -------\n");

	struct file *mfile = file_reopen(file);
	void *ori_addr = addr;
	size_t read_bytes = (length > file_length(mfile)) ? file_length(mfile) : length;
	// size_t zero_bytes = PGSIZE - read_bytes % PGSIZE;
	size_t zero_bytes = ((length + PGSIZE-1)/PGSIZE)*PGSIZE - read_bytes;
	
	if (addr == NULL
	|| (file == NULL)
	|| (addr >= KERN_BASE)
	|| (addr + length >= KERN_BASE)
	|| ((long)length <= 0)
	|| (read_bytes + zero_bytes) % PGSIZE != 0
	|| (pg_ofs(addr) != 0)
	|| (offset % PGSIZE != 0)
	) {
		// /**/printf("------- do_mmap end NULL handling -------\n");
		return NULL;
	}
	while (read_bytes > 0 || zero_bytes > 0) {
		/* Do calculate how to fill this page.
			* We will read PAGE_READ_BYTES bytes from FILE
			* and zero the final PAGE_ZERO_BYTES bytes. */
		size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
		size_t page_zero_bytes = PGSIZE - page_read_bytes;

		/* TODO: Set up aux to pass information to the lazy_load_segment. */
		struct container *container = (struct container *)malloc(sizeof(struct container));
		container->file = mfile;
		container->page_read_bytes = page_read_bytes;
		container->offset = offset;
		container->page_zero_bytes=page_zero_bytes;

		if (!vm_alloc_page_with_initializer (VM_FILE, addr,
					writable, lazy_load_segment, container)){
			// /**/printf("------- do_mmap end false -------\n");
			free(container);
			return NULL;
		}

		/* Advance. */
		read_bytes -= page_read_bytes;
		zero_bytes -= page_zero_bytes;
		addr += PGSIZE;
		offset += page_read_bytes;
	}
	// /**/printf("------- do_mmap end -------\n");
	return ori_addr;
}

/* Do the munmap */
void do_munmap(void *addr) {
	// /**/printf("------- do_munmap -------\n");
    struct thread *curr = thread_current();
    struct page *page;
    while ((page = spt_find_page(&curr->spt, addr))) {
		// printf("\npage va : %p", page->va);
		if(page){
        	destroy(page);
			hash_delete(&curr->spt.spt_hash, &page->elem);
			// spt_remove_page(&curr->spt, page);
			// vm_dealloc_page(page);
		}
		addr += PGSIZE;
	}
	// /**/printf("------- do_munmap end -------\n");
}