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
	// /**/printf("\n------- vm_init -------");
	vm_anon_init ();
	vm_file_init ();
#ifdef EFILESYS  /* For project 4 */
	pagecache_init ();
#endif
	register_inspect_intr ();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
	
	list_init(&frame_table);
	// /**/printf("\n------- vm_init end -------");
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type (struct page *page) {
	// /**/printf("\n------- page_get_type -------");
	int ty = VM_TYPE (page->operations->type);
	switch (ty) {
		case VM_UNINIT:
			// /**/printf("\n------- page_get_type end VM_UNINIT -------");
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

	// /**/printf("\n------- vm_alloc_page_with_initializer -------");
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
		// /**/printf("\n------- vm_alloc_page_with_initializer end -------");
		return spt_insert_page(spt, page);
	}
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
struct page *
spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) {
	/* TODO: Fill this function. */
	// /**/printf("\n------- spt_find_page -------");
	struct page page;
	page.va = pg_round_down(va);

	struct hash_elem *e = hash_find(&spt->spt_hash, &page.elem);

	if (e) {
		// /**/printf("\n------- spt_find_page end hash_elem not null -------");
		return hash_entry(e, struct page, elem);
	} else {
		// /**/printf("\n------- spt_find_page end hash_elem null -------");
		return NULL;
	}
}

/* Insert PAGE into spt with validation. */
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED,
		struct page *page UNUSED) {
	// /**/printf("\n------- spt_insert_page -------");
	int succ = false;
	/* TODO: Fill this function. */
	struct hash_elem *elem = hash_insert(&spt->spt_hash, &page->elem);

	if (!elem) {	// 넣을 수 있으면
		succ = true;
	}
	
	// /**/printf("\n------- spt_insert_page end -------");
	return succ;
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	// /**/printf("\n------- spt_remove_page -------");
	hash_delete(&spt->spt_hash, &page->elem);
	vm_dealloc_page (page);
	// /**/printf("\n------- spt_remove_page end -------");
	return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim (void) {
	// /**/printf("\n------- vm_get_victim -------");
	struct frame *victim = NULL;
	/* TODO: The policy for eviction is up to you. */

	// /**/printf("\n------- vm_get_victim end -------");
	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	// /**/printf("\n------- vm_evict_frame -------");
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */

	// mytodo : 프레임 스왑 아웃 및 반환. 희생 프레임을 받아서 스왑 아웃

	// /**/printf("\n------- vm_evict_frame end -------");
	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame (void) {
	// /**/printf("\n------- vm_get_frame -------");
	struct frame *frame = NULL;
	/* TODO: Fill this function. */
	// PJ3
	frame = (struct frame *)malloc(sizeof (struct frame));
	frame->kva = palloc_get_page(PAL_USER);
	
	if (frame->kva == NULL) {
		// frame = vm_evict_frame();
		// frame->page = NULL;
		
		// return frame;
		// /**/printf("\n------- vm_get_frame end kva NULL -------");
		return NULL;
	}
	
	// 생성된 프레임을 frame_table에 넣어준다.
	list_push_back (&frame_table, &frame->elem);
	frame->page = NULL;

	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);
	// /**/printf("\n------- vm_get_frame end -------");
	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
	// /**/printf("\n------- vm_stack_growth -------");
	bool success = false;
	void *stack_max = (void *)((uint8_t *)thread_current()->stack_max - PGSIZE);
	
	if(vm_alloc_page(VM_ANON | VM_MARKER_0, stack_max, 1)){
        success = vm_claim_page(stack_max);
        if(success){
            thread_current()->stack_max = stack_max;
        }
    }
	// /**/printf("\n------- vm_stack_growth end -------");
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
	// /**/printf("\n------- vm_handle_wp -------");
	// /**/printf("\n------- vm_handle_wp end -------");
}

/* Return true on success */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
		bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
    struct supplemental_page_table *spt UNUSED = &thread_current()->spt;
    struct page *page = NULL;

	// /**/printf("\n------- vm_try_handle_fault -------");
    if (addr == NULL){
		// /**/printf("\n------- vm_try_handle_fault end (addr == NULL) -------");
        return false;
	}

    if (is_kernel_vaddr(addr)){
		// /**/printf("\n------- vm_try_handle_fault end is_kernel_vaddr(addr) -------");
        return false;
	}

	// mytodo : stack_pointer - 8 <= addr 이게 뭔솔?
	void *stack_pointer = user ? f->rsp : thread_current()->stack_pointer;
	if (stack_pointer - 8 <= addr && addr >= STACK_LIMIT && addr <= USER_STACK) {
		vm_stack_growth(addr);
		// /**/printf("\n------- vm_try_handle_fault end (stack_pointer - 8 <= addr && addr >= STACK_LIMIT && addr <= USER_STACK) -------");
		return true;
	}
	// if (thread_current()->stack_max > addr) {
	// 	vm_stack_growth(addr);
	// }

    if (not_present) // 접근한 메모리의 physical page가 존재하지 않은 경우
    {
        /* TODO: Validate the fault */
        page = spt_find_page(spt, addr);
        if (page == NULL){
			// /**/printf("\n------- vm_try_handle_fault end (page == NULL) -------");
            return false;
		}
        if (write == 1 && page->writable == 0){ // write 불가능한 페이지에 write 요청한 경우
			// /**/printf("\n------- vm_try_handle_fault end (write == 1 && page->writable == 0) -------");
            return false;
		}
		// /**/printf("\n------- vm_try_handle_fault end (not_present) -------");
        return vm_do_claim_page(page);
    }
	// /**/printf("\n------- vm_try_handle_fault end false -------");
    return false;
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page (struct page *page) {
	// /**/printf("\n------- vm_dealloc_page -------");
	destroy (page);
	free (page);
	// /**/printf("\n------- vm_dealloc_page end -------");
}

/* Claim the page that allocate on VA. */
bool
vm_claim_page (void *va UNUSED) {
	/* 역할: 이 함수는 주어진 가상 주소(virtual address)에 대해 페이지를 할당하고, 
	그에 따른 프레임을 설정하는 역할을 합니다. 페이지 테이블에 새로운 매핑을 설정하여 가상 주소가 물리 메모리의 프레임에 매핑되도록 합니다. */
	// /**/printf("\n------- vm_claim_page -------");
	struct page *page = spt_find_page(&thread_current()->spt, va);
	/* TODO: Fill this function */
	// PJ3
	
	if (page == NULL) {
		// /**/printf("\n------- vm_claim_page end (page == NULL) -------");
		return false;
	}

	// /**/printf("\n------- vm_claim_page end -------");
	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page(struct page *page)
{
	// /**/printf("\n------- vm_do_claim_page -------");
	/* 역할: 이 함수는 페이지를 할당하고 MMU(Memory Management Unit)를 설정하는 역할을 합니다. 
	페이지와 프레임 간의 연결을 설정하고, 페이지 테이블 엔트리를 업데이트하여 가상 주소를 물리 주소로 변환할 수 있도록 합니다. */
	// PJ3
	if (!page || !is_user_vaddr(page->va)) {
		return false;
	}
	struct frame *frame = vm_get_frame();
	
	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* 페이지의 VA를 프레임의 PA에 매핑하기 위해 PTE insert */
	if (!pml4_set_page(thread_current()->pml4, page->va, frame->kva, page->writable)) {
		// /**/printf("\n------- vm_do_claim_page end (!pml4_set_page(thread_current()->pml4, page->va, frame->kva, page->writable))-------");
		return false;
	}
	// /**/printf("\n------- vm_do_claim_page end page->frame->kva -------");
	return swap_in(page, frame->kva);
}

/* Initialize new supplemental page table */
void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {
	// /**/printf("\n------- supplemental_page_table_init -------");
	hash_init(&spt->spt_hash, hs_hash_func, hs_less_func, NULL);
	// /**/printf("\n------- supplemental_page_table_init end -------");
}

/* Copy supplemental page table from src to dst */
bool supplemental_page_table_copy(struct supplemental_page_table *dst UNUSED, struct supplemental_page_table *src UNUSED) {
	// /**/printf("\n------- supplemental_page_table_copy -------");
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
				// mytodo : 어째서 VM_UNINIT 타입을 다른 타입으로 변경한 후에 초기화 하는지 의문
				if (!vm_alloc_page_with_initializer(page_get_type(src_page), upage, writable, src_page->uninit.init, src_page->uninit.aux)){
					// /**/printf("\n------- supplemental_page_table_copy end (!vm_alloc_page_with_initializer(page_get_type(src_page), upage, writable, src_page->uninit.init, src_page->uninit.aux)) -------");
					goto err;
				}
				break;

			case VM_ANON:                                   // src 타입이 anon인 경우
				if (!vm_alloc_page(type, upage, writable)){  // UNINIT 페이지 생성 및 초기화
					// /**/printf("\n------- supplemental_page_table_copy end (!vm_alloc_page(type, upage, writable)) -------");
					goto err;
				}
				if (!vm_claim_page(upage)){  // 물리 메모리와 매핑하고 initialize
					// /**/printf("\n------- supplemental_page_table_copy end (!vm_claim_page(upage)) -------");
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
				// /**/printf("\n------- supplemental_page_table_copy end default -------");
				goto err;
		}
	}
	// /**/printf("\n------- supplemental_page_table_copy end true -------");
	return true;

err:
	return false;
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
	// /**/printf("\n------- supplemental_page_table_kill -------");

	// mytodo : dirty bit가 1이면 저장공간 내용 수정
	// hash_destroy(&spt->spt_hash, action_func);
	hash_clear(&spt->spt_hash, action_func);
	// /**/printf("\n------- supplemental_page_table_kill end -------");
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
