#include "mfs.h"
#include "udp.h"

//get buffer
char buffer[4096];

//sock addr struct
struct sockaddr_in addr;
int fd;
int portNum;
struct timeval timeCheck;
fd_set rfds;

int MFS_Init(char *hostname, int port) {
    portNum = port;
    fd = UDP_Open(0);
    assert(fd > -1);
    
    int rc = UDP_FillSockAddr(&addr, hostname, portNum);
    assert(rc == 0);
    
    return rc;
}

int MFS_Lookup(int pinum, char *name) {
    int valReturn;
    
    //initialize message
    msg_t msg;
    msg.block = -1;
    msg.inum = -1;
    msg.pinum = pinum;
    msg.type = -1;
    msg.lib = LOOKUP;
    //get name
    memcpy(msg.name, name, sizeof(msg.name));
    sprintf(msg.buffer, "Hello");
    msg.returnNum = -1;
    
    valReturn = 0;
    int rc;
    
    //wait until one gets a returned value
    while(valReturn == 0) {
        rc = UDP_Write(fd, &addr, (char *) &msg, sizeof(msg_t));
        
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        
        //wait for 5 seconds
        timeCheck.tv_sec = 5;
        timeCheck.tv_usec = 0;
        
        valReturn = select(fd + 1, &rfds, NULL, NULL, &timeCheck);
        
        
        if(valReturn) {
            if(rc > 0) {
                struct sockaddr_in retaddr;
                //grab back return val from server
                rc = UDP_Read(fd, &retaddr, (char*) &msg, sizeof(msg_t));
            }
        }
    }
    
    return msg.returnNum;
}


//returns infomation about data given
int MFS_Stat(int inum, MFS_Stat_t *m) {
    
    int valReturn;
    
    msg_t msg;
    msg.inum = inum;
    msg.block = -1;
    msg.lib = STAT;
    msg.pinum = -1;
    msg.type = -1;
    msg.returnNum -1;
    
    int rc;
    valReturn = 0;
    
    //wait for 5 sec and write values in
    while(valReturn == 0) {
        rc = UDP_Write(fd, &addr, (char *) &msg, sizeof(msg_t));
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        
        timeCheck.tv_sec = 5;
        timeCheck.tv_usec = 0;
        valReturn = select(fd + 1, &rfds, NULL, NULL, &timeCheck);
        
        if(valReturn) {
            if(rc > 0) {
                struct sockaddr_in retaddr;
                rc = UDP_Read(fd, &retaddr, (char*)&msg, sizeof(msg_t));
            }
        }
    }
    
    m->type = msg.stat.type;
    m->size = msg.stat.size;
    
    return msg.returnNum;
}



//write new
int MFS_Write(int inum, char *buffer, int block) {
    
    int valReturn;
    msg_t msg;
    msg.lib = WRITE;
    msg.inum = inum;
    msg.block = block;
    memcpy(msg.buffer, buffer, 4096);
    //int i;
   // for(i = 0; i < 4096; i++)
    //{
    //    printf("%c ", msg.buffer[i]);
    //}
    int rc;
    
    valReturn = 0;
    while(valReturn == 0) {
        rc = UDP_Write(fd, &addr, (char*)&msg, sizeof(msg_t));
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        timeCheck.tv_sec = 5;
        timeCheck.tv_usec = 0;
        valReturn = select(fd + 1, &rfds, NULL, NULL, &timeCheck);
        
        if(valReturn) {
            if(rc > 0) {
                struct sockaddr_in retaddr;
                rc = UDP_Read(fd, &retaddr, (char *)&msg, sizeof(msg_t));
            }
        }
        
    }
    
    return msg.returnNum;
    
}

int MFS_Read(int inum, char *buffer, int block) {
    int valReturn;
    msg_t msg;
    
    msg.inum = inum;
    sprintf(msg.buffer, buffer);
    msg.block = block;
    msg.lib = READ;
    msg.type = -1;
    msg.pinum = -1;
    msg.returnNum = -1;
    
    int rc;
    
    valReturn = 0;
    while(valReturn == 0) {
        rc = UDP_Write(fd, &addr, (char*)&msg, sizeof(msg_t));
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        timeCheck.tv_sec = 5;
        timeCheck.tv_usec = 0;
        valReturn = select(fd + 1, &rfds, NULL, NULL, &timeCheck);
        
        if(valReturn) {
            if(rc > 0) {
                struct sockaddr_in retaddr;
                rc = UDP_Read(fd, &retaddr, (char *)&msg, sizeof(msg_t));
                printf("read: %s\n", msg.buffer);
            }
        }
        
    }
    
    memcpy(buffer, msg.buffer, 4096);
    return msg.returnNum;
}

int MFS_Creat(int pinum, int type, char *name) {
    int valReturn;
    msg_t msg;
    
    sprintf(msg.name, name);
    msg.type = type;
    msg.pinum = pinum;
    msg.lib = CREAT;
    msg.inum = -1;
    msg.block = -1;
    msg.returnNum = -1;
    
    int rc;
    
    valReturn = 0;
    while(valReturn == 0) {
        rc = UDP_Write(fd, &addr, (char*)&msg, sizeof(msg_t));
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        timeCheck.tv_sec = 5;
        timeCheck.tv_usec = 0;
        valReturn = select(fd + 1, &rfds, NULL, NULL, &timeCheck);
        
        if(valReturn) {
            if(rc > 0) {
                struct sockaddr_in retaddr;
                rc = UDP_Read(fd, &retaddr, (char *)&msg, sizeof(msg_t));
            }
        }
        
    }
    
    return msg.returnNum;
}

int MFS_Unlink(int pinum, char *name) {
    int valReturn;
    msg_t msg;
    
    sprintf(msg.name, name);
    msg.pinum = pinum;
    msg.lib = UNLINK;
    msg.inum = -1;
    msg.type = -1;
    msg.block = -1;
    msg.returnNum = -1;
    
    int rc;
    
    valReturn = 0;
    while(valReturn == 0) {
        rc = UDP_Write(fd, &addr, (char*)&msg, sizeof(msg_t));
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        timeCheck.tv_sec = 5;
        timeCheck.tv_usec = 0;
        valReturn = select(fd + 1, &rfds, NULL, NULL, &timeCheck);
        
        if(valReturn) {
            if(rc > 0) {
                struct sockaddr_in retaddr;
                rc = UDP_Read(fd, &retaddr, (char *)&msg, sizeof(msg_t));
            }
        }
        
    }
    
    return msg.returnNum;
    
}

int MFS_Shutdown() {
    
    msg_t msg;
    msg.lib = SHUTDOWN;
    msg.returnNum = 0;
    int rc;
    rc = UDP_Write(fd, &addr, (char *)&msg, sizeof(msg_t));
    if(rc > 0) {
        struct sockaddr_in retaddr;
        rc = UDP_Read(fd, &retaddr, (char *)&msg, sizeof(msg_t));
    }
    
    return 0;
}


