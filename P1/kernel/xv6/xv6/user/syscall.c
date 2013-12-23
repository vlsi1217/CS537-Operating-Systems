#include "types.h"
#include "stat.h"
#include "user.h"

char buf[512];

void
syscall(void)
{
    printf(1, "number of syscall: %d\n", getsyscallinfo());
}

int
main(int argc, char *argv[])
{
	syscall();
	exit();
}
