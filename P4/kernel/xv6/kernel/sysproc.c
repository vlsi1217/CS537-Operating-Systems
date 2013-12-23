#include "types.h"
#include "x86.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"
#include "sysfunc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return proc->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = proc->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;
  
  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(proc->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since boot.
int
sys_uptime(void)
{
  uint xticks;
  
  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int
sys_clone(void)
{
  void (*fcn)(void*);
  void* arg;
  void* stack;
  if(argptr(0, (void*)&fcn, sizeof(void*)) < 0)
    return -1;
  if(argptr(1, (void*)&arg, sizeof(void*)) < 0)
    return -1;
  if(argptr(2, (void*)&stack, sizeof(void*)) < 0)
    return -1;
  
  
  return clone(fcn, arg, stack);
}

int sys_join(void)
{
  void **stack = NULL;
  if(argptr(0, (void*)&stack, sizeof(void**)) < 0)
  {
    return -1;
  }
  
  return join(stack);
}

int sys_condsleep(void)
{
  int n;
  void *lockVoid;
  if(argint(0, &n) < 0)
    return -1;
  if(argptr(0, (void*)&lockVoid, sizeof(void*)) < 0)
    return -1;
  
  lock_t *lock = (lock_t*)lockVoid;
  //acquire(&guardlock);
  acquire(&cvlock);
  xchg(&lock->locked, 0);
  int sleep = condsleep(&n, &cvlock);
  release(&cvlock);
  return sleep;
}

int sys_condwakeup(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  return condwakeup(&n);
}
