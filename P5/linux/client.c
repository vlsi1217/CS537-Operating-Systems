#include <stdio.h>
#include "udp.h"
#include "mfs.h"

#define BUFFER_SIZE (4096)
char buffer[BUFFER_SIZE];

int
main(int argc, char *argv[])
{
    
    char *host[1024];
    //null terminated
    host[1023] = '\0';
    gethostname((char*)host, 1023);
    
    int rc = MFS_Init(host, 10000);
    
    assert(rc == 0);
    
    rc = MFS_Creat(0, 1, "dir1\0");

    char *buff = malloc(4096);
    sprintf(buff, "srart block");
    rc = MFS_Write(1, buff, 0);
    assert(rc == 0);
    
    char *retBuff = malloc(4096);
    rc = MFS_Read(1, retBuff, 0);
    if(strcmp(retBuff, buff) != 0) {
        exit(-1);
    }
    
    
    int i;
    for(i = 0; i < 4096; i++) {
        if(retBuff[i] != buff[i]) {
            exit(-1);
        }
    }
    
    char str[4096 * 2];
    
    //write down num
    for(i = 0; i < 420; i++) {
        sprintf(str + strlen(str), "%d\n", i);
    }
    rc = MFS_Write(1, str, 2);
    assert(rc == 0);
    
    rc = MFS_Read(1, retBuff, 2);
    
    if(strcmp(retBuff, str) != 0) {
        return -1;
    }
    
    rc = MFS_Shutdown();

    return 0;
}


