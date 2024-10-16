/* uninit.c: Implementation of uninitialized page.
 *
 * All of the pages are born as uninit page. When the first page fault occurs,
 * the handler chain calls uninit_initialize (page->operations.swap_in).
 * The uninit_initialize function transmutes the page into the specific page
 * object (anon, file, page_cache), by initializing the page object,and calls
 * initialization callback that passed from vm_alloc_page_with_initializer
 * function.
 * */

#include "vm/vm.h"
#include "vm/uninit.h"

static bool uninit_initialize (struct page *page, void *kva);
static void uninit_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations uninit_ops = {
	.swap_in = uninit_initialize,
	.swap_out = NULL,
	.destroy = uninit_destroy,
	.type = VM_UNINIT,
};

/* DO NOT MODIFY this function */
void
uninit_new (struct page *page, void *va, vm_initializer *init,
		enum vm_type type, void *aux,
		bool (*initializer)(struct page *, enum vm_type, void *)) {
	// /**/printf("------- uninit_new -------\n");
	ASSERT (page != NULL);

	*page = (struct page) {
		.operations = &uninit_ops,
		.va = va,
		.frame = NULL, /* no frame for now */
		.uninit = (struct uninit_page) {
			.init = init,
			.type = type,
			.aux = aux,
			.page_initializer = initializer,
		}
	};
	// /**/printf("------- uninit_new end -------\n");
}

/* Initalize the page on first fault */
static bool
uninit_initialize (struct page *page, void *kva) {
	// /**/printf("------- uninit_initialize -------\n");
	// 페이지 초기화
	struct uninit_page *uninit = &page->uninit;

	/* Fetch first, page_initialize may overwrite the values */
	vm_initializer *init = uninit->init;
	void *aux = uninit->aux;

	/* TODO: You may need to fix this function. */
	// /**/printf("------- uninit_initialize end -------\n");
	return uninit->page_initializer (page, uninit->type, kva) &&
		(init ? init (page, aux) : true);
}

/* uninit_page가 보유한 자원을 해제합니다. 대부분의 페이지는 다른 페이지 객체로 변환되지만,
 * 프로세스가 종료할 때 실행 중 한 번도 참조되지 않은 uninit 페이지가 남아있을 수 있습니다.
 * PAGE는 호출자에 의해 해제될 것입니다. */
static void
uninit_destroy (struct page *page) {
	// /**/printf("------- uninit_destroy -------\n");
	struct uninit_page *uninit UNUSED = &page->uninit;
	/* TODO: 이 함수를 채우세요.
	 * TODO: 처리할 것이 없다면, 그냥 return 하세요. */
	// /**/printf("------- uninit_destroy end -------\n");
	return;
}
