/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"

#include "include/threads/mmu.h"

struct list frame_table;

/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void
vm_init (void) {
	// /**/printf("------- vm_init -------\n");
	vm_anon_init ();
	vm_file_init ();
#ifdef EFILESYS  /* For project 4 */
	pagecache_init ();
#endif
	register_inspect_intr ();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
	
	list_init(&frame_table);
	// /**/printf("------- vm_init end -------\n");
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type (struct page *page) {
	// /**/printf("------- page_get_type -------\n");
	int ty = VM_TYPE (page->operations->type);
	switch (ty) {
		case VM_UNINIT:
			// /**/printf("------- page_get_type end VM_UNINIT -------\n");
			return VM_TYPE (page->uninit.type);
		default:
			return ty;
	}
}

/* Helpers */
static struct frame *vm_get_victim (void);
static bool vm_do_claim_page (struct page *page);
static struct frame *vm_evict_frame (void);

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init, void *aux) {

	// /**/printf("------- vm_alloc_page_with_initializer -------\n");
	ASSERT (VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current ()->spt;

	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page (spt, upage) == NULL) {
		// 유저 페이지가 아직 없으니까 초기화를 해줘야 한다!
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */
		struct page *page = (struct page *)malloc(sizeof(struct page));
	
		typedef bool (*initializerFunc)(struct page *, enum vm_type, void *);
		initializerFunc initializer = NULL;

		switch(VM_TYPE(type)){
			case VM_ANON:
				initializer = anon_initializer;
				break;
			case VM_FILE:
				initializer = file_backed_initializer;
				break;
		}
		uninit_new(page, upage, init, type, aux, initializer);

		// page member 초기화
		page->writable = writable;
		/* TODO: Insert the page into the spt. */
		// /**/printf("------- vm_alloc_page_with_initializer end -------\n");
		return spt_insert_page(spt, page);
	}
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
struct page *
spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) {
	/* TODO: Fill this function. */
	// /**/printf("------- spt_find_page -------\n");
	struct page page;
	page.va = pg_round_down(va);

	struct hash_elem *e = hash_find(&spt->spt_hash, &page.elem);

	if (e) {
		// /**/printf("------- spt_find_page end hash_elem not null -------\n");
		return hash_entry(e, struct page, elem);
	} else {
		// /**/printf("------- spt_find_page end hash_elem null -------\n");
		return NULL;
	}
}

/* Insert PAGE into spt with validation. */
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED,
		struct page *page UNUSED) {
	// /**/printf("------- spt_insert_page -------\n");
	int succ = false;
	/* TODO: Fill this function. */
	struct hash_elem *elem = hash_insert(&spt->spt_hash, &page->elem);

	if (!elem) {	// 넣을 수 있으면
		succ = true;
	}
	
	// /**/printf("------- spt_insert_page end -------\n");
	return succ;
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	// /**/printf("------- spt_remove_page -------\n");
	hash_delete(&spt->spt_hash, &page->elem);
	vm_dealloc_page (page);
	// /**/printf("------- spt_remove_page end -------\n");
	return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim (void) {
	/* TODO: The policy for eviction is up to you. */
	// /**/printf("------- vm_get_victim -------\n");
	struct list_elem *e = list_pop_front(&frame_table);
	struct frame *victim = list_entry(e, struct frame, elem);
	// /**/printf("------- vm_get_victim end -------\n");
	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	// /**/printf("------- vm_evict_frame -------\n");
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */
	bool success = swap_out(victim->page);
	if (!success) {
		return NULL;
	}

	// mytodo : 프레임 스왑 아웃 및 반환. 희생 프레임을 받아서 스왑 아웃

	// /**/printf("------- vm_evict_frame end -------\n");
	return victim;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame (void) {
	// /**/printf("------- vm_get_frame -------\n");
	struct frame *frame = NULL;
	/* TODO: Fill this function. */
	frame = (struct frame *)malloc(sizeof (struct frame));
	frame->kva = palloc_get_page(PAL_USER | PAL_ZERO);
	
	if (frame->kva == NULL) {
		frame = vm_evict_frame();
		// /**/printf("------- vm_get_frame end kva NULL -------\n");
	}
	frame->page = NULL;
	list_push_back (&frame_table, &frame->elem);
	
	// 생성된 프레임을 frame_table에 넣어준다.
	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);
	// /**/printf("------- vm_get_frame end -------\n");
	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
	// /**/printf("------- vm_stack_growth -------\n");
	bool success = false;
	void *stack_max = thread_current()->stack_max - PGSIZE;
	
	if(vm_alloc_page(VM_ANON | VM_MARKER_0, stack_max, 1)){
		success = vm_claim_page(stack_max);
		if(success){
			thread_current()->stack_max = stack_max;
		}
	}
	// /**/printf("------- vm_stack_growth end -------\n");
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
	// /**/printf("------- vm_handle_wp -------\n");
	// /**/printf("------- vm_handle_wp end -------\n");
}

/* Return true on success */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
		bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
	struct supplemental_page_table *spt UNUSED = &thread_current()->spt;
	struct page *page = spt_find_page(&thread_current()->spt, addr);

	/* TODO: Validate the fault */
	if (addr == NULL || is_kernel_vaddr(addr))
		return false;

	/** Project 3: Copy On Write (Extra) - 접근한 메모리의 page가 존재하고 write 요청인데 write protected인 경우라 발생한 fault일 경우*/
	if (!not_present && write)
	    return false;

	/** Project 3: Copy On Write (Extra) - 이전에 만들었던 페이지인데 swap out되어서 현재 spt에서 삭제하였을 때 stack_growth 대신 claim_page를 하기 위해 조건 분기 */
	if (!page) {
		/** Project 3: Stack Growth - stack growth로 처리할 수 있는 경우 */
		/* stack pointer 아래 8바이트는 페이지 폴트 발생 & addr 위치를 USER_STACK에서 1MB로 제한 */
		void *stack_pointer = user ? f->rsp : thread_current()->stack_pointer;
		if (stack_pointer - 8 <= addr && addr >= STACK_LIMIT && addr <= USER_STACK) {
			vm_stack_growth(addr);
			return true;
		}
		return false;
	}

	return vm_do_claim_page(page);  // demand page 수행
}


/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page (struct page *page) {
	// /**/printf("------- vm_dealloc_page -------\n");
	destroy (page);
	free (page);
	// /**/printf("------- vm_dealloc_page end -------\n");
}

/* Claim the page that allocate on VA. */
bool
vm_claim_page (void *va UNUSED) {
	/* 역할: 이 함수는 주어진 가상 주소(virtual address)에 대해 페이지를 할당하고, 
	그에 따른 프레임을 설정하는 역할을 합니다. 페이지 테이블에 새로운 매핑을 설정하여 가상 주소가 물리 메모리의 프레임에 매핑되도록 합니다. */
	// /**/printf("------- vm_claim_page -------\n");
	struct page *page = spt_find_page(&thread_current()->spt, va);
	/* TODO: Fill this function */
	if (page == NULL) {
		// /**/printf("------- vm_claim_page end (page == NULL) -------\n");
		return false;
	}

	// /**/printf("------- vm_claim_page end -------\n");
	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page(struct page *page)
{
	// /**/printf("------- vm_do_claim_page -------\n");
	/* 역할: 이 함수는 페이지를 할당하고 MMU(Memory Management Unit)를 설정하는 역할을 합니다. 
	페이지와 프레임 간의 연결을 설정하고, 페이지 테이블 엔트리를 업데이트하여 가상 주소를 물리 주소로 변환할 수 있도록 합니다. */
	if (!page || !is_user_vaddr(page->va)) {
		return false;
	}
	struct frame *frame = vm_get_frame();
	
	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* 페이지의 VA를 프레임의 PA에 매핑하기 위해 PTE insert */
	if (!pml4_set_page(thread_current()->pml4, page->va, frame->kva, page->writable)) {
		// /**/printf("------- vm_do_claim_page end (!pml4_set_page(thread_current()->pml4, page->va, frame->kva, page->writable))-------\n");
		return false;
	}
	// /**/printf("------- vm_do_claim_page end page->frame->kva -------\n");
	return swap_in(page, frame->kva);
}

/* Initialize new supplemental page table */
void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {
	// /**/printf("------- supplemental_page_table_init -------\n");
	hash_init(&spt->spt_hash, hs_hash_func, hs_less_func, NULL);
	// /**/printf("------- supplemental_page_table_init end -------\n");
}

/* Copy supplemental page table from src to dst */
bool supplemental_page_table_copy(struct supplemental_page_table *dst UNUSED, struct supplemental_page_table *src UNUSED) {
	// /**/printf("------- supplemental_page_table_copy -------\n");
	struct hash_iterator iter;
	struct page *dst_page;
	struct aux *aux;

	hash_first(&iter, &src->spt_hash);

	while (hash_next(&iter)) {
		struct page *src_page = hash_entry(hash_cur(&iter), struct page, elem);
		enum vm_type type = src_page->operations->type;
		void *upage = src_page->va;
		bool writable = src_page->writable;

		switch (type) {
			case VM_UNINIT:  // src 타입이 initialize 되지 않았을 경우
				if (!vm_alloc_page_with_initializer(page_get_type(src_page), upage, writable, src_page->uninit.init, src_page->uninit.aux)){
					// /**/printf("------- supplemental_page_table_copy end (!vm_alloc_page_with_initializer(page_get_type(src_page), upage, writable, src_page->uninit.init, src_page->uninit.aux)) -------\n");
					goto err;
				}
				break;

			case VM_ANON:                                   // src 타입이 anon인 경우
				if (!vm_alloc_page(type, upage, writable)){  // UNINIT 페이지 생성 및 초기화
					// /**/printf("------- supplemental_page_table_copy end (!vm_alloc_page(type, upage, writable)) -------\n");
					goto err;
				}
				if (!vm_claim_page(upage)){  // 물리 메모리와 매핑하고 initialize
					// /**/printf("------- supplemental_page_table_copy end (!vm_claim_page(upage)) -------\n");
					goto err;
				}
				struct page *dst_page = spt_find_page(dst, upage);  // 대응하는 물리 메모리 데이터 복제
				memcpy(dst_page->frame->kva, src_page->frame->kva, PGSIZE);

				/** Project 3: Copy On Write (Extra) - 메모리에 load된 데이터를 write하지 않는 이상 똑같은 메모리를 사용하는데
				 *  2개의 복사본을 만드는 것은 메모리가 낭비가 난다. 따라서 write 요청이 들어왔을 때만 해당 페이지에 대한 물리메모리를
				 *  할당하고 맵핑하면 된다. */
				// if (!vm_copy_claim_page(dst, upage, src_page->frame->kva, writable))  // 물리 메모리와 매핑하고 initialize
				//     goto err;

				break;

			case VM_FILE:  // src 타입이 FILE인 경우
				if (!vm_alloc_page_with_initializer(type, upage, writable, NULL, &src_page->file))
					goto err;

				dst_page = spt_find_page(dst, upage);  // 대응하는 물리 메모리 데이터 복제
				if (!file_backed_initializer(dst_page, type, NULL))
					goto err;

				dst_page->frame = src_page->frame;
				if (!pml4_set_page(thread_current()->pml4, dst_page->va, src_page->frame->kva, src_page->writable))
					goto err;

				break;

			default:
				// /**/printf("------- supplemental_page_table_copy end default -------\n");
				goto err;
		}
	}
	// /**/printf("------- supplemental_page_table_copy end true -------\n");
	return true;

err:
	return false;
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
	// /**/printf("------- supplemental_page_table_kill -------\n");

	// mytodo : dirty bit가 1이면 저장공간 내용 수정
	hash_clear(&spt->spt_hash, action_func);
	// /**/printf("------- supplemental_page_table_kill end -------\n");
}


/*------- Project3 VM -------*/
/* Custom destructor function to handle page writeback and cleanup. */

void action_func(struct hash_elem *e, void *aux) {
	struct page *page = hash_entry(e, struct page, elem);
	destroy(page);
	free(page);
}

uint64_t hs_hash_func(const struct hash_elem *e, void *aux UNUSED) {
	const struct page *page = hash_entry(e, struct page, elem);
	return hash_bytes(&page->va, sizeof page->va);
}

bool hs_less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED) {
	const struct page *page_a = hash_entry(a, struct page, elem);
	const struct page *page_b = hash_entry(b, struct page, elem);
	return (page_a->va) < (page_b->va);
}
