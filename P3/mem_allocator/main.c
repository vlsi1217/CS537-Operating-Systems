#include <stdio.h>
#include <assert.h>
#include "mem.h"

int main(int argc, char* argv[])
{
	printf("mem init status: %d\n", Mem_Init(4096));
	char* ptr = Mem_Alloc(100, FIRSTFIT);
	char* ptr2 = Mem_Alloc(100, BESTFIT);
	assert(ptr != NULL);
	Mem_Free((void*)ptr);
	char* ptr3 = Mem_Alloc(200, WORSTFIT);	
	Mem_Free(ptr2);
	Mem_Free(ptr3);
	Mem_Dump();
	return 0;
}
