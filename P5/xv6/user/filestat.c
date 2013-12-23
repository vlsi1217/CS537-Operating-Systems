#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  int fd;

  if(argc != 2){
    printf(1, "Usage: filestat [filepath]\n");
    exit();
  }

  struct stat st;
  if((fd = open(argv[1], 0)) < 0){
      printf(1, "cat: cannot open %s\n", argv[1]);
      exit();
  }

  if(fstat(fd, &st) < 0){
    printf(2, "filestat: cannot stat %s\n", argv[1]);
    close(fd);
    exit();
  }
  switch (st.type) {
  case T_DIR:
    printf(1, "File type: %s\n", "Directory");
    break;
  case T_FILE:
    printf(1, "File type: %s\n", "File");
    break;
  case T_DEV:
    printf(1, "File type: %s\n", "Special device");
    break;
  case T_CHECKED:
    printf(1, "File type: %s\n", "File with checksum feature");
    break;
  }
  printf(1, "size: %d\n", st.size);
  printf(1, "checksum: %d\n", st.checksum);
  close(fd);
  exit();
}
