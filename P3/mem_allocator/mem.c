#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <assert.h>
#include "mem.h"

int m_error;
int policies;
int initialized = 0;
int totalSize = 0;

typedef struct node{
	int size;
	int free;
	struct node *next;
	struct node *prev;
} header;

header* freeList = NULL;
header* memList = NULL;
header* begin = NULL;

void coalesce();
header* firstfit(int size);
header* bestfit(int size);
header* worstfit(int size);

int Mem_Init(int sizeOfRegion)
{
	if(initialized || sizeOfRegion <= 0)
	{
		m_error = E_BAD_ARGS;
		return -1;
	}
	int requestSize = sizeOfRegion;
	int pageSize = getpagesize();
	if(sizeOfRegion % pageSize != 0)
	{
		requestSize += (pageSize - sizeOfRegion % pageSize);
	}
	//printf("request size: %d\n", requestSize);
	int fd = open("/dev/zero", O_RDWR);
	freeList = (header*) mmap(NULL, sizeOfRegion, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	if(freeList == MAP_FAILED)
	{
		m_error = E_BAD_ARGS;
		return -1;
	}
	header free = {requestSize - (int)sizeof(header), 1, NULL, NULL};
	*freeList = free;
	close(fd);
	initialized = 1;
	totalSize = requestSize;
	begin = freeList;
	return 0;
}

void *Mem_Alloc(int size, int policy)
{
	int requestSize = size;
	header* freeChunkHeader = NULL;
	if(requestSize % 8 != 0)
	{
		requestSize += (8 - (size % 8));
	}
	//printf("request size: %d\n", requestSize);
	//Mem_Dump();
	switch(policy)
	{
	case BESTFIT: 
		if((freeChunkHeader = bestfit(requestSize)) == NULL)
		{
			m_error = E_NO_SPACE;
			return NULL;
		}
		break;
	case WORSTFIT:
		if((freeChunkHeader = worstfit(requestSize)) == NULL)
		{
			m_error = E_NO_SPACE;
			return NULL;
		}
		break;
	case FIRSTFIT:
		if((freeChunkHeader = firstfit(requestSize)) == NULL)
		{
			m_error = E_NO_SPACE;
			return NULL;
		}
		break;
	}
	//determine if we need to split the free chunk
	header* newMemHeader = freeChunkHeader;
	if(freeChunkHeader->size - requestSize - (int)sizeof(header) > 0)
	{
		header newFreeChunk = {freeChunkHeader->size - requestSize - (int)sizeof(header), 1, NULL, NULL};
		header* newFreeHeader = (header*) ((char*) freeChunkHeader + requestSize + (int)sizeof(header));
		*newFreeHeader = newFreeChunk;
		newFreeHeader->next = freeList;
		newFreeHeader->prev = NULL;
		if(freeList != NULL)
		{
			freeList->prev = newFreeHeader;
		}
		freeList = newFreeHeader;
		newMemHeader->size = requestSize;
	}
	if(newMemHeader->next != NULL)
	{
		newMemHeader->next->prev = newMemHeader->prev;
	}
	if(newMemHeader->prev != NULL)
	{
		newMemHeader->prev->next = newMemHeader->next;
	}
	if(newMemHeader == freeList)
	{
		freeList = newMemHeader->next;
	}
	newMemHeader->next = memList;
	if(memList != NULL)
	{
		memList->prev = newMemHeader;
	}
	newMemHeader->prev = NULL;
	memList = newMemHeader;
	memList->free = 0;
	return (void*) ((char*)newMemHeader + (int)sizeof(header));
}

int Mem_Free(void* ptr)
{
	if(ptr == NULL)
	{
		return -1;
	}
	header* memTmp = memList;
	while(memTmp != NULL && (void*)((char*)memTmp + (int)sizeof(header)) != ptr)
	{
		memTmp = memTmp->next;
	}
	if(memTmp == NULL)
	{
		m_error = E_BAD_POINTER; 
		return -1;
	}
	
	if(memTmp->prev != NULL)
	{
		memTmp->prev->next = memTmp->next;
	}
	if(memTmp->next != NULL)
	{
		memTmp->next->prev = memTmp->prev;
	}
	if(memList == memTmp)
	{
		memList = memTmp->next;
	}
	memTmp->free = 1;
	coalesce();
	return 0;
}

void Mem_Dump()
{
	header* freeTmp = freeList;
	header* memTmp = memList;
	int size = 0;
	printf("Size of header: %lu\n", sizeof(header));
	printf("Here is the mem list: \n");
	while(memTmp != NULL)
	{
		size += memTmp->size;
		printf("At %p, this memory chunk has been allocated for %d freed? %d\n", memTmp, memTmp->size, memTmp->free);
		memTmp = memTmp->next;
	}
	printf("Here is the free list: \n");
	while(freeTmp != NULL)
	{
		size += freeTmp->size;
		printf("At %p, this free chunk has %d freed? %d\n", freeTmp,freeTmp->size, freeTmp->free);
		freeTmp = freeTmp->next;
	}
	printf("total size is: %d totalSize is: %d\n", size, totalSize);
}

void coalesce()
{
	header* curr = begin;		//beginning of the list
	header* prev = NULL;
	int currSize = 0, immediate = 0;
	while(currSize < totalSize)
	{
		if(curr->free)
		{
			if(prev == NULL)
			{
				freeList = curr;
				freeList->prev = NULL;
				freeList->next = NULL;
				prev = curr;
			}	
			else
			{
				if(!immediate)
				{
					prev->next = curr;
					curr->prev = prev;
					prev = curr;
				}
				else
				{
					prev->size += curr->size + (int)sizeof(header);
					curr->free = 1;
				}
			}
			immediate = 1;
		}
		else
		{
			immediate = 0;
		}
		currSize += curr->size + (int)sizeof(header);	
		curr = (header*) ((char*) curr + (int)sizeof(header) + curr->size);
	}
	if(prev != NULL)
	{
		prev->next = NULL;
	}
}

header* firstfit(int size)
{
	header* freeTmp = freeList;
	while(freeTmp != NULL)
	{
		if(freeTmp->size >= size)
		{
			return freeTmp;
		}
		freeTmp = freeTmp->next;
	}
	return freeTmp;
}

header* bestfit(int size)
{
	header* freeTmp = freeList; 
	header* bestTmp = freeList;
	int first = 1;
	while(freeTmp != NULL)
	{
		if(first && freeTmp->size >= size)
		{	
			bestTmp = freeTmp;
			first = 0;			
		}	
		if(freeTmp->size >= size && freeTmp->size < bestTmp->size)
		{
			bestTmp = freeTmp;
		}
		freeTmp = freeTmp->next;
	}
	if(bestTmp == freeList && bestTmp->size < size)
	{
		return NULL;
	}
	return bestTmp;
}

header* worstfit(int size)
{
	header* worstTmp = freeList;
	header* freeTmp = freeList;
	int first = 1;
	while(freeTmp != NULL)
	{
		if(first && freeTmp->size >= size)
		{	
			worstTmp = freeTmp;
			first = 0;			
		}
		if(freeTmp->size >= size && freeTmp->size > worstTmp->size)
		{
			worstTmp = freeTmp;
		}
		freeTmp = freeTmp->next;
	}
	if(worstTmp == freeList && worstTmp->size < size)
	{
		return NULL;
	}
	return worstTmp;
}
