/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "threads/vaddr.h"
#include "userprog/process.h"
#include "userprog/syscall.h"

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
	// /**/printf("\n------- vm_file_init -------");
	// /**/printf("\n------- vm_file_init end -------");
}

/* Initialize the file backed page */
bool
file_backed_initializer (struct page *page, enum vm_type type, void *kva) {
	// /**/printf("\n------- file_backed_initializer -------");
	/* Set up the handler */
	page->operations = &file_ops;

    struct file_page *file_page = &page->file;

    struct container *container = (struct container *)page->uninit.aux;
    file_page->file = container->file;
    file_page->offset = container->offset;
    file_page->page_read_bytes = container->page_read_bytes;

    return true;
	// /**/printf("\n------- file_backed_initializer end -------");
}

/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in (struct page *page, void *kva) {
	// /**/printf("\n------- file_backed_swap_in -------");
	struct file_page *file_page UNUSED = &page->file;
	// /**/printf("\n------- file_backed_swap_in end -------");
}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out (struct page *page) {
	// /**/printf("\n------- file_backed_swap_out -------");
	struct file_page *file_page UNUSED = &page->file;
	// /**/printf("\n------- file_backed_swap_out end -------");
}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy (struct page *page) {
	// /**/printf("\n------- file_backed_destroy -------");
	struct file_page *file_page UNUSED = &page->file;

    if (pml4_is_dirty(thread_current()->pml4, page->va)) {
        file_write_at(file_page->file, page->va, file_page->page_read_bytes, file_page->offset);
        pml4_set_dirty(thread_current()->pml4, page->va, false);
    }

    if (page->frame) {
        list_remove(&page->frame->elem);
        // page->frame->page = NULL;
        page->frame = NULL;
		// palloc_free_page(page->frame->kva);
        free(page->frame);
    }

    pml4_clear_page(thread_current()->pml4, page->va);
	// free(page);
	// /**/printf("\n------- file_backed_destroy end -------");
}

static bool
lazy_load_segment (struct page *page, void *aux) {
	/* TODO: Load the segment from the file */
	/* TODO: This called when the first page fault occurs on address VA. */
	/* TODO: VA is available when calling this function. */
	// project 3
	// /**/printf("\n------- lazy_load_segment -------");
	struct file *file = ((struct container *)aux)->file;
	off_t offsetof = ((struct container *)aux)->offset;
	size_t page_read_bytes = ((struct container *)aux)->page_read_bytes;
	size_t page_zero_bytes = PGSIZE - page_read_bytes;

	file_seek(file, offsetof);
	// 여기서 file을 page_read_bytes만큼 읽어옴
	if(file_read(file, page->frame->kva, page_read_bytes) != (int)page_read_bytes){
		palloc_free_page(page->frame->kva);
		// /**/printf("\n------- lazy_load_segment end false -------");
		return false;
	}
	// 나머지 0을 채우는 용도
	memset(page->frame->kva + page_read_bytes, 0, page_zero_bytes);

	// /**/printf("\n------- lazy_load_segment end true -------");
	return true;
}

/* Do the mmap */
void *
do_mmap (void *addr, size_t length, int writable,
		struct file *file, off_t offset) {
	// /**/printf("\n------- do_mmap -------");

	struct file *mfile = file_reopen(file);
	void *ori_addr = addr;
	size_t read_bytes = (length > file_length(mfile)) ? file_length(mfile) : length;
	// size_t zero_bytes = PGSIZE - read_bytes % PGSIZE;
	size_t zero_bytes = length - read_bytes;
	
	if (addr == NULL
	|| (file == NULL)
	|| (addr >= KERN_BASE)
	// || (addr + length < USER_STACK)
	|| (addr + length >= KERN_BASE)
	|| ((long)length <= 0)
	|| (read_bytes + zero_bytes) % PGSIZE != 0
	|| (pg_ofs(addr) != 0)
	|| (offset % PGSIZE != 0)
	) return NULL;
	while (read_bytes > 0 || zero_bytes > 0) {
		/* Do calculate how to fill this page.
			* We will read PAGE_READ_BYTES bytes from FILE
			* and zero the final PAGE_ZERO_BYTES bytes. */
		size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
		size_t page_zero_bytes = PGSIZE - page_read_bytes;
		// /**/printf("\npage_read_bytes : %d", page_read_bytes);
		// /**/printf("\npage_zero_bytes : %d", page_zero_bytes);

		/* TODO: Set up aux to pass information to the lazy_load_segment. */
		// project 3
		// void *aux = NULL;
		struct container *container = (struct container *)malloc(sizeof(struct container));
		container->file = mfile;
		container->page_read_bytes = page_read_bytes;
		container->offset = offset;
		container->page_zero_bytes=page_zero_bytes;

		if (!vm_alloc_page_with_initializer (VM_FILE, addr,
					writable, lazy_load_segment, container)){
			// /**/printf("\n------- load_segment end false -------");
			free(container);
			return NULL;
		}

		/* Advance. */
		read_bytes -= page_read_bytes;
		zero_bytes -= page_zero_bytes;
		addr += PGSIZE;
		offset += page_read_bytes;
		// /**/printf("\nread_bytes : %d", read_bytes);
		// /**/printf("\nzero_bytes : %d", zero_bytes);
	}
	// /**/printf("\n------- do_mmap end -------");
	return ori_addr;
}

/* Do the munmap */
// void
// do_munmap (void *addr) {
//	// /**/printf("\n------- do_munmap -------");
// 	struct thread *curr = thread_current();
// 	struct hash_iterator iter;

// 	hash_first(&iter, &curr->spt.spt_hash);

// 	while (hash_next(&iter)) {
// 		struct page *curr_page = hash_entry(hash_cur(&iter), struct page, elem);
// 		destroy(curr_page);
// 	}
//	// /**/printf("\n------- do_munmap end -------");
// }
void do_munmap(void *addr) {
	// /**/printf("\n------- do_munmap -------");
    struct thread *curr = thread_current();
    struct page *page;
    lock_acquire(&syscall_lock);
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
    lock_release(&syscall_lock);
	// /**/printf("\n------- do_munmap end -------");
}