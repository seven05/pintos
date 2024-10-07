#include <syscall.h>

int main (int, char *[]);
void _start (int argc, char *argv[]);

//PROJECT CODE FLOW 1
void
_start (int argc, char *argv[]) {
	exit (main (argc, argv));
}
