#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}


int
sys_exit(void)
{
  int status;
  if(argint(0, &status) < 0)
    return -1;
  //cprintf("%d", status);
  exit(status);
  return 0;  // not reached
}

int
sys_priority(void)
{
  int int_priority;
  if(argint(0, &int_priority) < 0)
    return -1;
  priority(int_priority);
  return 0; 
}

int
sys_policy(void)
{
  int policyNum;
  if(argint(0, &policyNum) < 0)
    return -1;
  priority(policyNum);
  return 0; 
}

int
sys_wait(void)
{
  int status;
  if(argint(0, &status) < 0)
    return -1;
  return wait((int*)status);
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
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
