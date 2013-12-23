#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>

//store the number of records in each key/record pair
#define NUMRECS (24)

//structure defined for the actual record
typedef struct __rec_t {
    unsigned int key;
    unsigned int record[NUMRECS];
} rec_t;

/**
 * Comparator for sorting
 */
int cmpfunc (const void * a, const void * b)
{
   return ( (*(rec_t*)a).key - (*(rec_t*)b).key);
}

/**
 * print out the usage error and exit
 */
void usage() 
{
    fprintf(stderr, "Usage: fastsort -i inputfile -o outputfile\n");
    exit(1);
}

/**
 * print out the file not found error and exit
 */
void fileNotFound(char* fileName)
{
	fprintf(stderr, "Error: Cannot open file %s\n", fileName);
	exit(1);
}

int main(int argc, char *argv[])
{
    // arguments
    char *inFile = "/no/such/file";
    char *outFile = "/no/such/file";
    // input params
    int c;
    int verify = 0;
    opterr = 0;
    //against the none argument case
    if (argc == 1)
    {
    	usage();
    }
    while ((c = getopt(argc, argv, "i:o:")) != -1) {
		switch (c) {
			case 'i':
				verify++;
	    		inFile = strdup(optarg);
	    		break;
			case 'o':
				verify++;
				outFile = strdup(optarg);
				break;
			case '?':
				if (c == 'i' || c == 'o')
				{
					usage();
				}
			default:
	    		usage();
		}
	}
	if(verify != 2)
	{
		usage();
	}
	struct stat fileStat;
	if(stat(inFile, &fileStat) < 0){   
        fileNotFound(inFile);
    }
    int fileSize = fileStat.st_size;
	//first open the outfile
	int outFd = open(outFile, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);
    if (outFd < 0) {
		fileNotFound(outFile);
    }
    //next open the infile
    int inFd = open(inFile, O_RDONLY);
    if (inFd < 0) {
		fileNotFound(inFile);
    }
    int counter = 0;
    rec_t* records = (rec_t*) malloc(fileSize);
    if (records == NULL)
    {
    	fprintf(stderr, "Memory allocation failed!\n");
    	exit(0);
    }
    while ((counter + 1) * sizeof(rec_t) <= fileSize) {	
		int rc;
		rc = read(inFd, &(records[counter++]), sizeof(rec_t));
		if (rc == 0) // 0 indicates EOF
	    	break;
		if (rc < 0) {
	    	fprintf(stderr, "Read failed!\n");
	    	free(records);
	    	close(inFd);
    		close(outFd);
	    	exit(1);
		}
	}
	qsort(records, counter, sizeof(rec_t), cmpfunc);
	counter = 0;
	while ((counter + 1) * sizeof(rec_t) <= fileSize) {	 
		int rc = write(outFd, &(records[counter++]), sizeof(rec_t));
		if (rc != sizeof(rec_t)) {
	    	fprintf(stderr, "Write failed!\n");
	    	free(records);
	    	close(inFd);
    		close(outFd);
	    	exit(1);
		}
    }
    free(records);
    close(inFd);
    close(outFd);
    return 0;
}
