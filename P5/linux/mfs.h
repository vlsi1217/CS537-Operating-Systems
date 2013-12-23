#ifndef __MFS_h__
#define __MFS_h__

#define MFS_DIRECTORY    (0)
#define MFS_REGULAR_FILE (1)

#define MFS_BLOCK_SIZE   (4096)

typedef struct __MFS_Stat_t {
    int type;   // MFS_DIRECTORY or MFS_REGULAR
    int size;   // bytes
    // note: no permissions, access times, etc.
} MFS_Stat_t;

typedef struct __MFS_DirEnt_t {
    char name[60];  // up to 60 bytes of name in directory (including \0)
    int  inum;      // inode number of entry (-1 means entry not used)
} MFS_DirEnt_t;

//define a checkpoint
//256 places in checkpoint it must point to
typedef struct checkpoint_t {
    int imap[256];
    int endLog; //end of file
} checkpoint_t;

//inode
typedef struct inode_t {
    MFS_Stat_t stat;
    int blockArr[14];
} inode_t;

//imap that will contain pointers to inodes
typedef struct imap_t {
    int inodeArr[16];
} imap_t;

//contain directories
typedef struct dir_t {
    MFS_DirEnt_t dirArr[64];
} dir_t;

//this will contain all inodes
typedef struct inodeArr_t {
    int inodeArr[4096];
} inodeArr_t;

//use for library
typedef enum lib_t {
    INIT,
    LOOKUP,
    STAT,
    WRITE,
    READ,
    CREAT,
    UNLINK,
    SHUTDOWN
} lib_t;

typedef enum type_t {
    REG, //use for regular types
    DIR //use for directory types
} type_t;

//send this struct over the server
typedef struct msg_t {
    char name[60]; //name of the directory
    char buffer[4096]; //buffer
    int inum;
    int block;
    int type;
    int returnNum;
    int pinum;
    MFS_Stat_t stat;
    lib_t lib;
    int regDir; //type of file
} msg_t;

int MFS_Init(char *hostname, int port);
int MFS_Lookup(int pinum, char *name);
int MFS_Stat(int inum, MFS_Stat_t *m);
int MFS_Write(int inum, char *buffer, int block);
int MFS_Read(int inum, char *buffer, int block);
int MFS_Creat(int pinum, int type, char *name);
int MFS_Unlink(int pinum, char *name);
int MFS_Shutdown();

#endif // __MFS_h__

