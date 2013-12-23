#include <stdio.h>
#include "udp.h"
#include "mfs.h"

#define BUFFER_SIZE (4096)

//functions
int initImage(char *imgName);
int loadMem();
int sRead(int inum, char *buff, int blk);
int sWrite(int inum, char *buff, int blk);
int sUnlink(int pinum, char *name);
int sStat(int inum, MFS_Stat_t *m);
int sLookup(int pinum, char *name);
int sCreate(int pinum, int type, char *name);
int cImapPiece();
int delInode(int inum);
int delImap(int imapInd);
int cInode(int pinum, int type);
void handle(char * msgBuff);
int shutDown();

int sd;
struct sockaddr_in s;
char name[256];
int fdDisk;
checkpoint_t chkpt;
inodeArr_t iArr;

int
main(int argc, char *argv[])
{
    if(argc != 3) {
        fprintf(stderr, "Usage: server <port> <file image>\n");
        exit(0);
    }
    
    int portNum = atoi(argv[1]);
    sprintf(name, argv[2]);
    
    //get port open
    
    sd = UDP_Open(portNum);
    assert(sd > -1);
    
    printf("SERVER:: waiting in loop\n");
    
    //init the server image
    initImage(name);
    loadMem();
    
    while (1) {
        struct sockaddr_in n;
        s = n;
        char buffer[sizeof(msg_t)];
        int rc = UDP_Read(sd, &s, buffer, sizeof(msg_t));
        if (rc > 0) {
            handle(buffer);
            rc = UDP_Write(sd, &s, buffer, sizeof(msg_t));
        }
    }
    
    return 0;
}

int initImage(char *imgName) {
    fdDisk = open(imgName, O_RDWR);
    
    //doesn't exit
    if(fdDisk < 0) {
        fdDisk = open(imgName, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
        
        
        //add all sizes that are needed the first time initializing
        chkpt.endLog = sizeof(checkpoint_t) + sizeof(imap_t) + sizeof(inode_t) + sizeof(dir_t);
        
        int i;
        
        //make all peices in checkpoint invalid
        for(i = 0; i < 256; i++) {
            chkpt.imap[i] = -1;
        }
        
        //the first imap is at right after the checkpoint so add checkpoint size
        chkpt.imap[0] = sizeof(checkpoint_t);
        
        lseek(fdDisk, 0, 0);
        write(fdDisk, &chkpt, sizeof(checkpoint_t));
        
        //create the root
        imap_t imap;
        //clear out inode arrays
        for(i = 0; i < 16; i++) {
            imap.inodeArr[i] = -1;
        }
        
        //point the first to right after the imap
        imap.inodeArr[0] = sizeof(checkpoint_t)+sizeof(imap_t);
        
        write(fdDisk, &imap, sizeof(imap));
        
        //now write roote inode
        inode_t root;
        //make it a directory
        root.stat.type = 0;
        //initialize all to -1
        for(i = 0; i < 14; i++) {
            root.blockArr[i] = -1;
        }
        
        //point data block to right after inode
        root.blockArr[0] = sizeof(checkpoint_t) + sizeof(imap_t) + sizeof(inode_t);
        root.stat.size = 2 * 4096;
        write(fdDisk, &root, sizeof(inode_t));
        
        //write first directory block
        dir_t rootDir;
        int sent = sizeof(dir_t)/sizeof(rootDir.dirArr[0]);
        for(i = 0; i < sent; i++) {
            rootDir.dirArr[i].inum = -1;
            sprintf(rootDir.dirArr[i].name, "\0");
        }
        
        //add . and ..
        sprintf(rootDir.dirArr[0].name, ".\0");
        //make it valid
        rootDir.dirArr[0].inum = 0;
        
        sprintf(rootDir.dirArr[1].name, "..\0");
        //make it valid
        rootDir.dirArr[1].inum = 0;
        //write dir block
        write(fdDisk, &rootDir, sizeof(dir_t));
    }
    else {
        //read the disk image's checkpoint region
        read(fdDisk, &chkpt, sizeof(checkpoint_t));
    }
    
    int i;
    for(i = 0; i < 4096; i++) {
        iArr.inodeArr[i] = -1;
    }
    
    //now read imap
    int j = 0;
    int k = 0;
    imap_t imapTemp;
    while(chkpt.imap[i] >= 0) {
        lseek(fdDisk, chkpt.imap[i], 0);
        read(fdDisk, &imapTemp, sizeof(imap_t));
        while(imapTemp.inodeArr[k] >= 0) {
            iArr.inodeArr[j] = imapTemp.inodeArr[k];
            j++;
            k++;
        }
        ;
    }
    
    
    //load mem
    loadMem();
    
    return 0;
}

//load mem
int loadMem() {
    //first go and read checkpoint region
    lseek(fdDisk, 0, 0);
    read(fdDisk, &chkpt, sizeof(checkpoint_t));
    
    //load imap into mem
    int i = 0;
    int j = 0;
    int k = 0;
    
    for(i = 0; i < 4096; i++) {
        iArr.inodeArr[i] = -1;
    }
    
    imap_t imapTemp;
    for(i = 0; i < 256; i++) {
        if(chkpt.imap[i] >= 0) {
            lseek(fdDisk, chkpt.imap[i], 0);
            read(fdDisk, &imapTemp, sizeof(imap_t));
            for(j = 0; j < 16; j++) {
                if(imapTemp.inodeArr[j] >= 0) {
                    iArr.inodeArr[k] = imapTemp.inodeArr[j];
                    k++;
                }
            }
            
        }
    }
    
    return 0;
}

int sRead(int inum, char *buff, int blk) {
    if(inum >= 4096 || inum < 0) {
        return -1;
    }
    
    if(blk > 13 || blk < 0) {
        return -1;
    }
    
    loadMem();
    
    if(iArr.inodeArr[inum] == -1) {
        return -1;
    }
    
    inode_t inode;
    lseek(fdDisk, iArr.inodeArr[inum], 0);
    read(fdDisk, &inode, sizeof(inode_t));
    
    if(inode.blockArr[blk] == -1) {
        return -1;
    }
    
    lseek(fdDisk, inode.blockArr[blk], 0);
    read(fdDisk, buff, 4096);
    return 0;
}

int sWrite(int inum, char *buff, int blk) {
    if(inum >= 4096 || inum < 0) {
        return -1;
    }
    
    if(blk > 13 || blk < 0) {
        return -1;
    }
    
    loadMem();
    
    if(iArr.inodeArr[inum] == -1) {
        return -1;
    }
    
    //read inode
    inode_t inode;
    lseek(fdDisk, iArr.inodeArr[inum], 0);
    read(fdDisk, &inode, sizeof(inode_t));
    
    if(inode.stat.type != 1) {
        return -1;
    }
    
    if(inode.blockArr[blk] == -1) {
        int endLogTmp = chkpt.endLog;
        dir_t nDirBlk;
        int i;
        for(i = 0; i < 64; i++) {
            sprintf(nDirBlk.dirArr[i].name, "\0");
            nDirBlk.dirArr[i].inum = -1;
        }
        lseek(fdDisk, chkpt.endLog, 0);
        write(fdDisk, &nDirBlk, sizeof(dir_t));
        
        chkpt.endLog += 4096;
        lseek(fdDisk, 0, 0);
        write(fdDisk, &chkpt, sizeof(checkpoint_t));
        
        inode.blockArr[blk] = endLogTmp;
        inode.stat.size = (blk + 1) * 4096;
        lseek(fdDisk, iArr.inodeArr[inum], 0);
        write(fdDisk, &inode, sizeof(inode_t));
        lseek(fdDisk, endLogTmp, 0);
    }
    else {
        inode.stat.size = (blk + 1) * 4096;
        lseek(fdDisk, iArr.inodeArr[inum], 0);
        write(fdDisk, &inode, sizeof(inode_t));
        lseek(fdDisk, inode.blockArr[blk], 0);
    }
    write(fdDisk, buff, 4096);
    printf("buffer: %s\n", buff);
    loadMem();
    return 0;
}

int sUnlink(int pinum, char *name) {
    loadMem();
    
    if(pinum < 0 || pinum > 4096) {
        return -1;
    }
    
    if(iArr.inodeArr[pinum] == -1) {
        return -1;
    }
    
    //test to see if name is equivalent to parent or current directory
    if(strcmp(name, "..\0") == 0 || strcmp(name, ".\0") == 0) {
        return -1;
    }
    
    if(strlen(name) > 60 || strlen(name) < 0) {
        return -1;
    }
    
    int pinumLoc = iArr.inodeArr[pinum];
    
    inode_t pInode;
    lseek(fdDisk, pinumLoc, 0);
    read(fdDisk, &pInode, sizeof(inode_t));

    if(pInode.stat.type != 0) {
        return -1;
    }
    
    int found = -1;
    int delDirBlkLoc;
    int delInd;
    int delInodeLoc;
    dir_t dirBlk;
    int i;
    int j;
    for(i = 0; i < 14; i++) {
        if(pInode.blockArr[i] >= 0) {
            lseek(fdDisk, pInode.blockArr[i], 0);
            read(fdDisk, &dirBlk, sizeof(dir_t));
            for(j = 0; j < 64; j++) {
                if(strcmp(dirBlk.dirArr[j].name, name) == 0) {
                    found = 1;
                    delInodeLoc = iArr.inodeArr[dirBlk.dirArr[j].inum];
                    delInd = j;
                    delDirBlkLoc = pInode.blockArr[i];
                    i = 15;
                    break;
                }
            }
        }
    }
    
    if(found < 0) {
        return -1;
    }
    
    inode_t inodeDel;
    lseek(fdDisk, delInodeLoc, 0);
    read(fdDisk, &inodeDel, sizeof(inodeDel));
    
    if(inodeDel.stat.type == 0) {
        if(inodeDel.stat.size > 2 * 4096) {
            return -1;
        }
    }

    delInode(delInodeLoc);
    
    dirBlk.dirArr[delInd].inum = -1;
    sprintf(dirBlk.dirArr[delInd].name, "\0");
    lseek(fdDisk, delDirBlkLoc, 0);
    write(fdDisk, &dirBlk, sizeof(dir_t));
    
    int sizeInd = 0;
    for(i = 0; i < 14; i++) {
        if(pInode.blockArr[i] != -1) {
            sizeInd = i;
        }
    }
    
    //increase size that can be put into directory
    pInode.stat.size = (sizeInd + 1) * 4096;
    lseek(fdDisk, pinumLoc, 0);
    write(fdDisk, &pInode, sizeof(inode_t));
    loadMem();
    return 0;
}

int sStat(int inum, MFS_Stat_t *m) {
    
    //check for errors
    if(inum < 0 || inum >= 4096 || fdDisk < 0) {
        return -1;
    }
    loadMem();
    
    
    if(iArr.inodeArr[inum] == -1) {
        return -1;
    }
    
    inode_t inode;
    lseek(fdDisk, iArr.inodeArr[inum], 0);
    read(fdDisk, &inode, sizeof(inode_t));
    
    m->type = inode.stat.type;
    m->size = inode.stat.size;
    
    return 0;
}

int sLookup(int pinum, char *name) {
    //check pinum
    if(pinum < 0 || pinum >= 4096) {
        return -1;
    }
    loadMem();
    //check if pinum in iArr is valid
    if(iArr.inodeArr[pinum] == -1) {
        return -1;
    }
    
    ////check if the name is valid
    if(strlen(name) < 1 || strlen(name) > 60) {
        return -1;
    }

    int i;
    int j;
    int dirBlk;
    
    int pinumLoc = iArr.inodeArr[pinum];
    
    //lseek to pinum location
    lseek(fdDisk, pinumLoc, 0);
    
    inode_t pInode;
    read(fdDisk, &pInode, sizeof(pInode));
    
    //check if directory
    if(pInode.stat.type != 0) {
        return -1;
    }
    
    for(i = 0; i < 14; i++) {
        dirBlk = pInode.blockArr[i];
        lseek(fdDisk, dirBlk, 0);
        dir_t dirBlkTmp;
        read(fdDisk, &dirBlkTmp, sizeof(dir_t));
        for(j = 0; j < 64; j++) {
            if(strncmp(dirBlkTmp.dirArr[j].name, name, 60) == 0) {
                return dirBlkTmp.dirArr[j].inum;
            }
        }
    }
    
    //if can't find name, return -1
    return -1;
}

int sCreate(int pinum, int type, char *name) {
    //makesure it is ok
    if((type != 0 && type != 1) || pinum < 0 || pinum > 4096) {
        return -1;
    }
    
    //check name size too see if too long or too short
    if(strlen(name) < 1 || strlen(name) >= 60) {
        return -1;
    }
    
    if(iArr.inodeArr[pinum] == -1) {
        return -1;
    }
    
    if(sLookup(pinum, name) >= 0) {
        return 0;
    }
    
    //get pinum location
    int pinumLoc = iArr.inodeArr[pinum];
    lseek(fdDisk, pinumLoc, 0);
    inode_t pInode;
    read(fdDisk, &pInode, sizeof(inode_t));
    
    if(pInode.stat.type != 0) {
        return -1;
    }
    
    if(pInode.stat.size >= (4096 * 14 * 64)) {
        return -1;
    }
    
    int newInum = cInode(pinum, type);
    int iDirBlkInd = pInode.stat.size / (4096 * 64);
    
    if(iDirBlkInd > 14) {
        return -1;
    }

    int dirBlkInd = (pInode.stat.size/(4096) % 64);
    
    
    pInode.stat.size += 4096;
    
    if(dirBlkInd == 0) {
        pInode.blockArr[iDirBlkInd] = chkpt.endLog;
        dir_t newDirBlk;
        int i;
        for(i = 0; i < 64; i++) {
            sprintf(newDirBlk.dirArr[i].name, "\0");
            newDirBlk.dirArr[i].inum = -1;
        }
        
        lseek(fdDisk, chkpt.endLog, 0);
        write(fdDisk, &newDirBlk, sizeof(dir_t));
        
        chkpt.endLog += 4096;
        lseek(fdDisk, 0, 0);
        write(fdDisk, &chkpt, sizeof(checkpoint_t));
    }
    
    lseek(fdDisk, pinumLoc, 0);
    write(fdDisk, &pInode, sizeof(pInode));
    loadMem();
    lseek(fdDisk, pInode.blockArr[iDirBlkInd], 0);
    dir_t dirBlk;
    read(fdDisk, &dirBlk, sizeof(dir_t));
    
    int nInd = dirBlkInd;
    sprintf(dirBlk.dirArr[nInd].name, "\0");
    sprintf(dirBlk.dirArr[nInd].name, name, 60);
    dirBlk.dirArr[nInd].inum = newInum;
    
    //write to updated directory
    lseek(fdDisk, pInode.blockArr[iDirBlkInd], 0);
    write(fdDisk, &dirBlk, sizeof(dir_t));
    
    loadMem();
    return 0;
}

int cImapPiece() {
    loadMem();
    int nImapInd;
    
    int i;
    for(i = 0; i < 256; i++) {
        if(chkpt.imap[i] == -1) {
            nImapInd = i;
            break;
        }
    }
    
    chkpt.imap[nImapInd] = chkpt.endLog;
    imap_t nimap;
    for(i = 0; i < 16; i++) {
        nimap.inodeArr[i] = -1;
    }
    
    chkpt.endLog += sizeof(imap_t);
    lseek(fdDisk, 0, 0);
    write(fdDisk, &chkpt, sizeof(checkpoint_t));
    lseek(fdDisk, chkpt.imap[nImapInd], 0);
    write(fdDisk, &nimap, sizeof(imap_t));
    loadMem();
    return nImapInd;
}

int delInode(int inum) {
    int imapInd = inum / 16;
    int imapInIndex = inum % 16;
    if(imapInIndex < 0) {
        return -1;
    }
    
    imap_t imapTmp;
    lseek(fdDisk, chkpt.imap[imapInd], 0);
    read(fdDisk, &imapTmp, sizeof(imap_t));
    
    imapTmp.inodeArr[imapInIndex] = -1;
    int i = 0;
    while(imapTmp.inodeArr[i] > 0 && i < 16) {
        i++;
    }
    
    if(i == 0) {
        int test = delImap(imapInd);
    }
    else {
        lseek(fdDisk, chkpt.imap[imapInd], 0);
        write(fdDisk, &imapTmp, sizeof(imapTmp));
    }
    loadMem();
    return 0;
    
}

int delImap(int imapInd) {
    loadMem();
    chkpt.imap[imapInd] = -1;
    
    //write to disk
    lseek(fdDisk, 0, 0);
    write(fdDisk, &chkpt, sizeof(checkpoint_t));
    loadMem();
    return 0;
}

int cInode(int pinum, int type) {
    loadMem();
    int nInodeNum;

    int i;
    for(i = 0; i < 4096; i++) {
        if(iArr.inodeArr[i] == -1) {
            nInodeNum = i;
            break;
        }
    }
    
    int imapInd = nInodeNum / 16;
    int nImapInd = nInodeNum % 16;
    
    int imapPart;
    if(nImapInd == 0) {
        imapPart = cImapPiece();
    }
    
    imap_t imapTmp;
    lseek(fdDisk, chkpt.imap[imapInd], 0);
    read(fdDisk, &imapTmp, sizeof(imap_t));
    
    imapTmp.inodeArr[nImapInd] = chkpt.endLog;
    lseek(fdDisk, chkpt.imap[imapInd], 0);
    write(fdDisk, &imapTmp, sizeof(imap_t));
    
    inode_t nInode;
    nInode.stat.type = type;
    for(i = 0; i < 14; i++) {
        nInode.blockArr[i] = -1;
    }
    
    nInode.blockArr[0] = chkpt.endLog + sizeof(inode_t);
    if(type == 0) {
        nInode.stat.size = 2 * 4096;
    } else {
        nInode.stat.size = 0;
    }
    
    lseek(fdDisk, chkpt.endLog, 0);
    write(fdDisk, &nInode, sizeof(inode_t));
    
    chkpt.endLog += sizeof(nInode);
    
    if(type == 0) {
        dir_t dirBlk;
        int k = sizeof(dirBlk)/sizeof(dirBlk.dirArr[0]);
        for(i = 0; i < k; i++) {
            dirBlk.dirArr[i].inum = -1;
            sprintf(dirBlk.dirArr[i].name, "\0");
        }
        
        sprintf(dirBlk.dirArr[0].name, ".\0");
        dirBlk.dirArr[0].inum = nInodeNum;
        
        sprintf(dirBlk.dirArr[1].name, "..\0");
        dirBlk.dirArr[1].inum = pinum;
        
        write(fdDisk, &dirBlk, sizeof(dir_t));
        chkpt.endLog += sizeof(dir_t);
    }
    else {
        char *nBlk = malloc(4096);
        write(fdDisk, nBlk, 4096);
        free(nBlk);
        chkpt.endLog += 4096;
    }
    
    lseek(fdDisk, 0, 0);
    write(fdDisk, &chkpt, sizeof(checkpoint_t));
    loadMem();
    return nInodeNum;
}

void handle(char * msgBuff) {
    msg_t *msg = (msg_t*) msgBuff;
    msg->returnNum = -1;
    
    int returnNum;
    switch(msg->lib) {
        case INIT:
            returnNum = initImage(name);
            msg->returnNum = returnNum;
            break;
        case LOOKUP:
            returnNum = sLookup(msg->pinum, msg->name);
            msg->returnNum = returnNum;
            break;
        case STAT:
            returnNum = sStat(msg->inum, &(msg->stat));
            msg->returnNum = returnNum;
            break;
        case WRITE:
            returnNum = sWrite(msg->inum, msg->buffer, msg->block);
            msg->returnNum = returnNum;
            break;
        case READ:
            returnNum = sRead(msg->inum, msg->buffer, msg->block);
            msg->returnNum = returnNum;
            break;
        case CREAT:
            returnNum = sCreate(msg->pinum, msg->type, msg->name);
            msg->returnNum = returnNum;
            break;
        case UNLINK:
            returnNum = sUnlink(msg->pinum, msg->name);
            msg->returnNum = returnNum;
            break;
        case SHUTDOWN:
            msg->returnNum = 0;
            memcpy(msgBuff, msg, sizeof(*msg));
            UDP_Write(sd, &s, msgBuff, sizeof(msg_t));
            fsync(fdDisk);
            close(fdDisk);
            exit(0);
            break;
        default:
            msg->returnNum = -1;
            break;
    }
    memcpy(msgBuff, msg, sizeof(*msg));
}

//shutdown server
int shutDown() {
    close(fdDisk);
    exit(0);
    return 0;
}

