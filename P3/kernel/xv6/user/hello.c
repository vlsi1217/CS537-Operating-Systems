#include "types.h"
#include "user.h"

int
main(int argc, char* argv[])
{
	char* p = (char*) (640 * 1024 - 4096);
	*p = 'b';	
	printf(1, "%c\n", *p);
	int rc = fork();
	if(rc == 0)
	{
		printf(1, "child: %c\n", *p);
	}
	else if(rc > 0)
	{
		(void) wait();
		printf(1, "parent: %c\n", *p);
	}
	exit();
}
