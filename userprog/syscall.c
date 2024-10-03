#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"
#include "filesys/file.h"

void syscall_entry (void);
void syscall_handler (struct intr_frame *);

/* System call.
 *
 * Previously system call services was handled by the interrupt handler
 * (e.g. int 0x80 in linux). However, in x86-64, the manufacturer supplies
 * efficient path for requesting the system call, the `syscall` instruction.
 *
 * The syscall instruction works by reading the values from the the Model
 * Specific Register (MSR). For the details, see the manual. */

/* An open file. */
struct file {
	struct inode *inode;        /* File's inode. */
	off_t pos;                  /* Current position. */
	bool deny_write;            /* Has file_deny_write() been called? */
};

#define MSR_STAR 0xc0000081         /* Segment selector msr */
#define MSR_LSTAR 0xc0000082        /* Long mode SYSCALL target */
#define MSR_SYSCALL_MASK 0xc0000084 /* Mask for the eflags */
#define FD_MAX 128
void
syscall_init (void) {
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48  |
			((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t) syscall_entry);

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
			FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
}

void halt (void) {
	power_off();
}
void exit (int status) {
	thread_exit();
	return status
}
pid_t fork (const char *thread_name) {
	// return thread_create (thread_name, PRI_DEFAULT, __do_fork, thread_current ());
}
int exec (const char *cmd_line) {

	if (process_exec() < 0)
		return -1;

	
}
int wait (pid_t pid) {

}
bool create (const char *file, unsigned initial_size) {
	return filesys_create(file, initial_size);
}
bool remove (const char *file) {
	return filesys_remove(file);
}
int open (const char *file) {
	struct thread *t = thread_current();
	if (t->next_fd == FD_MAX) {
		return -1;
	}
	t->fd_table[t->next_fd] = filesys_open(file);
	int fd = t->next_fd;

	for (int i = 3; i <FD_MAX ; i ++) {	//왜 3부터 시작인가?	//FD_MAX == 128
		if (i == FD_MAX) {
			t->next_fd = i;
			return -1;
		}
		
		if (t->fd_table[i] == NULL) {	//왜 NULL과 같을 때 실행되나?
			t->next_fd = i;	
		}
	}

	return fd;
}
int filesize (int fd) {
	struct thread *t = thread_current();
	struct file *file = t->fd_table[fd];
	return file->inode->data.length;
}
int read (int fd, void *buffer, unsigned size) {

}
int write (int fd, const void *buffer, unsigned size)
{	
	printf("fd: %d, buffer: %s, size: %d\n",fd,(char *)buffer,size);
	if (fd == 0){
		return -1;
	}
	if (fd == 1){
		putbuf(buffer, size);
		return size;
	}
	if (fd == 2){
		putbuf(buffer, size);
		return size;
	}
	struct file *file = get_file_by_descriptor(fd);
	if (file == NULL){
		return -1;
	}
	int written = file_write(file, buffer, size);
	file->pos += written;
	return written;
}

struct file *get_file_by_descriptor(int fd)
{
	if (fd < 0 || fd > 128) return;
	
	struct thread *t = thread_current();

	return t->fd_table[fd];
}

void seek (int fd, unsigned position) {

}
unsigned tell (int fd) {
	struct thread *t = thread_current();
	return t->fd_table[fd]->pos;
}
void close (int fd) {
	struct thread *t = thread_current();
	free(t->fd_table[fd]);	//왜 free를 하나? open해서 돌기 때문?
	t->fd_table[fd] = NULL;
	if (t->next_fd == 128) {
		t->next_fd = fd;	
	}
}

/* The main system call interface */
void
syscall_handler (struct intr_frame *f UNUSED) {
	// TODO: Your implementation goes here.

	uint64_t arg1 = f->R.rdi;
	switch(f->R.rax) {
		case SYS_HALT:    
			print("halt\n");
			if(f->cs != 0) {
				halt();
				break;
			}               /* Halt the operating system. */
		case SYS_EXIT:                   /* Terminate this process. */
			print("exit\n");
			exit(arg1);
			
			break;
		case SYS_FORK:                   /* Clone current process. */
			print("fork\n");
			fork(arg1);
			
			break;
		case SYS_EXEC:                   /* Switch current process. */
			print("exec\n");
			exec(arg1);
			
			break;
		case SYS_WAIT:                   /* Wait for a child process to die. */
			print("wait\n");
			wait(arg1);
			
			break;
		case SYS_CREATE:                 /* Create a file. */
			print("create\n");
			create(arg1, sizeof(arg1));
			
			break;
		case SYS_REMOVE:                 /* Delete a file. */
			print("remove\n");
			remove(arg1);
			
			break;
		case SYS_OPEN:                   /* Open a file. */
			print("open\n");
			open(arg1);
			
			break;
		case SYS_FILESIZE:               /* Obtain a file's size. */
			print("filesize\n");
			filesize(arg1);
			
			break;
		case SYS_READ:                   /* Read from a file. */
			print("read\n");
			read(arg1, buf, sizeof(arg1));
			
			break;
		case SYS_WRITE:                  /* Write to a file. */
			print("write\n");
			
			write(arg1, arg2, arg3);
			
			break;
		case SYS_SEEK:                   /* Change position in a file. */
			print("seek\n");
			seek(arg1);
			
			break;
		case SYS_TELL:                   /* Report current position in a file. */
			print("tell\n");
			tell(arg1);
			
			break;
		case SYS_CLOSE:                  /* Close a file. */
			print("close\n");
			close(arg1);
			
			break;
		default:
			thread_exit();
			break; 
	}
	printf ("system call!\n");
	thread_exit ();
}

bool user_memory_valid(void *ptr) {
	struct thread *current = thread_current();
	uint64_t *pml4 = current->pml4;
	if (r != NULL && is_user_vaddr(r) && pml4_get_page(pml4, r) != NULL) {
		return true;
	}
	return false;
}
