/* inspect.c: Testing utility for VM. */
/* DO NOT MODIFY THIS FILE. */

#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/mmu.h"
#include "vm/inspect.h"

static void
inspect (struct intr_frame *f) {
	// /**/printf("------- inspect -------\n");
	const void *va = (const void *) f->R.rax;
	f->R.rax = PTE_ADDR (pml4_get_page (thread_current ()->pml4, va));
	// /**/printf("------- inspect end -------\n");
}

/* Tool for testing vm component. Calling this function via int 0x42.
 * Input:
 *   @RAX - Virtual address to inspect
 * Output:
 *   @RAX - Physical address that mmaped to input. */
void
register_inspect_intr (void) {
	// /**/printf("------- register_inspect_intr -------\n");
	intr_register_int (0x42, 3, INTR_OFF, inspect, "Inspect Virtual Memory");
	// /**/printf("------- register_inspect_intr end -------\n");
}
