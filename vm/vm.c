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
	vm_anon_init ();
	vm_file_init ();
#ifdef EFILESYS  /* For project 4 */
	pagecache_init ();
#endif
	register_inspect_intr ();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
	
	list_init(&frame_table);
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type (struct page *page) {
	int ty = VM_TYPE (page->operations->type);
	switch (ty) {
		case VM_UNINIT:
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

	// printf("\n------- vm_alloc_page_with_initializer -------\n\n");
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
		// printf("\n------- vm_alloc_page_with_initializer end -------\n\n");
		return spt_insert_page(spt, page);
	}
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
struct page *
spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) {
	/* TODO: Fill this function. */
	// printf("\n------- spt_find_page -------\n\n");
	struct page page;
	page.va = pg_round_down(va);

	struct hash_elem *e = hash_find(&spt->spt_hash, &page.elem);

	// printf("\n------- spt_find_page end -------\n\n");
	if (e) {
		return hash_entry(e, struct page, elem);
	} else {
		return NULL;
	}
}

/* Insert PAGE into spt with validation. */
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED,
		struct page *page UNUSED) {
	int succ = false;
	/* TODO: Fill this function. */
	struct hash_elem *elem = hash_insert(&spt->spt_hash, &page->elem);

	if (!elem) {	// 넣을 수 있으면
		succ = true;
	}

	return succ;
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	hash_delete(&spt->spt_hash, &page->elem);
	vm_dealloc_page (page);
	return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	/* TODO: The policy for eviction is up to you. */

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */

	// mytodo : 프레임 스왑 아웃 및 반환. 희생 프레임을 받아서 스왑 아웃

	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame (void) {
	struct frame *frame = NULL;
	/* TODO: Fill this function. */
	// PJ3
	frame = (struct frame *)malloc(sizeof (struct frame));
	frame->kva = palloc_get_page(PAL_USER);
	
	if (frame->kva == NULL) {
		// frame = vm_evict_frame();
		// frame->page = NULL;
		
		// return frame;
		return NULL;
	}
	
	// 생성된 프레임을 frame_table에 넣어준다.
	list_push_back (&frame_table, &frame->elem);
	frame->page = NULL;

	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);
	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* Return true on success */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
		bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
    struct supplemental_page_table *spt UNUSED = &thread_current()->spt;
    struct page *page = NULL;

	// printf("\n------- vm_try_handle_fault -------\n\n");
    if (addr == NULL){
		// printf("\naddr == NULL\n\n");
        return false;
	}

    if (is_kernel_vaddr(addr)){
		// printf("\nis_kernel_vaddr(addr)\n\n");
        return false;
	}

    if (not_present) // 접근한 메모리의 physical page가 존재하지 않은 경우
    {
        /* TODO: Validate the fault */
        page = spt_find_page(spt, addr);
        if (page == NULL){
			// printf("\npage == NULL\n\n");
            return false;
		}
        if (write == 1 && page->writable == 0){ // write 불가능한 페이지에 write 요청한 경우
		// printf("\nwrite == 1 && page->writable == 0\n\n");
            return false;
		}
		// printf("\nnot_present\n\n");
        return vm_do_claim_page(page);
    }
	// printf("\ngooooood\n\n");
    return false;
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

/* Claim the page that allocate on VA. */
bool
vm_claim_page (void *va UNUSED) {
	/* 역할: 이 함수는 주어진 가상 주소(virtual address)에 대해 페이지를 할당하고, 
	그에 따른 프레임을 설정하는 역할을 합니다. 페이지 테이블에 새로운 매핑을 설정하여 가상 주소가 물리 메모리의 프레임에 매핑되도록 합니다. */
	// printf("\n------- vm_claim_page -------\n\n");
	struct page *page = spt_find_page(&thread_current()->spt, va);
	/* TODO: Fill this function */
	// PJ3
	
	if (page == NULL) {
		// printf("\n------- vm_claim_page end page == NULL -------\n\n");
		return false;
	}

	// printf("\n------- vm_claim_page end good -------\n\n");
	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page(struct page *page)
{
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
		return false;
	}

	// printf("\n\n page->frame->kva\n\n");
	return swap_in(page, frame->kva);
}

/* Initialize new supplemental page table */
void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {
	hash_init(&spt->spt_hash, hs_hash_func, hs_less_func, NULL);
}

/* Copy supplemental page table from src to dst */
bool supplemental_page_table_copy(struct supplemental_page_table *dst UNUSED,
                                  struct supplemental_page_table *src UNUSED)
{
    struct hash_iterator i;
    hash_first(&i, &src->spt_hash);
    while (hash_next(&i))
    {
        // src_page 정보
        struct page *src_page = hash_entry(hash_cur(&i), struct page, elem);
        enum vm_type type = src_page->operations->type;
        void *upage = src_page->va;
        bool writable = src_page->writable;
	
		// mytodo: uninit을 복사할때 anon으로 하는 이유?
        /* 1) type이 uninit이면 */
        if (type == VM_UNINIT)
        { // uninit page 생성 & 초기화
            vm_initializer *init = src_page->uninit.init;
            void *aux = src_page->uninit.aux;
            vm_alloc_page_with_initializer(VM_ANON, upage, writable, init, aux);
            continue;
        }

        /* 2) type이 uninit이 아니면 */
        if (!vm_alloc_page(type, upage, writable)) // uninit page 생성 & 초기화
            // init이랑 aux는 Lazy Loading에 필요함
            // 지금 만드는 페이지는 기다리지 않고 바로 내용을 넣어줄 것이므로 필요 없음
            return false;

        // vm_claim_page으로 요청해서 매핑 & 페이지 타입에 맞게 초기화
        if (!vm_claim_page(upage))
            return false;

        // 매핑된 프레임에 내용 로딩
        struct page *dst_page = spt_find_page(dst, upage);

		// mytodo: 부모 프로세스와 자식 프로세스가 같은 물리 메모리를 바라보는 이유?
        memcpy(dst_page->frame->kva, src_page->frame->kva, PGSIZE);
    }
    return true;
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */

	// mytodo : dirty bit가 1이면 저장공간 내용 수정
	// hash_destroy(&spt->spt_hash, action_func);
	hash_clear(&spt->spt_hash, action_func);
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
