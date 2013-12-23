#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int
main(int argc, char *argv[])
{
  int fd1, fd2;

  if(argc != 3){
    printf(1, "Usage: test_fs [filepath1] [filepath2]\n");
    exit();
  }

  printf(1, "creating a normal file first: %s\n", argv[1]);

  if((fd1 = open(argv[1], O_CREATE|O_RDWR)) < 0){
      printf(1, "test_fs: cannot open %s\n", argv[1]);
      exit();
  }
  printf(1, "then write to normal file \n");
  if(write(fd1, "hello", 5) < 0)
  {
    printf(1, "write failed!\n");
  }
  else
  {
    printf(1, "write succeeded!\n");
  }

  printf(1, "then create a O_CHECKED file: %s\n", argv[2]);
  if((fd2 = open(argv[2], O_CREATE| O_CHECKED |O_RDWR)) < 0){
      printf(1, "test_fs: cannot open %s\n", argv[2]);
      exit();
  }

  printf(1, "then write to checked file \n");
  if(write(fd2, "hello", 5) < 0)
  {
    printf(1, "write failed!\n");
  }
  else
  {
    printf(1, "write succeeded!\n");
  }
  char buf1[10];
  char buf2[10];
  printf(1, "read from: %s\n", argv[1]);
  int rc;
  if((rc = read(fd1, buf1, 10)) != -1)
  {
    printf(1, "read is: %s! rc is: %d\n", buf1, rc);
  }
  else
  {
    printf(1, "read failed!\n");
  }
  printf(1, "read from: %s\n", argv[2]);
  if((rc = read(fd2, buf2, 10)) != -1)
  {
    printf(1, "read is: %s! rc is %d\n", buf2, rc);
  }
  else
  {
    printf(1, "read failed!\n");
  }
  close(fd1);
  close(fd2);
  exit();
}
