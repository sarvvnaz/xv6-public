#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
struct ptable_struct ptable ;
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
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
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
    if(myproc()->killed){
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
int
sys_create_palindrome(void){
	int num = myproc()->tf->ebx;
	cprintf("KERNEL: sys_create_palindrome() is called!\n",num);
	return create_palindrome(num);
}
int sys_sort_syscalls(void){
	int pid = myproc()->tf->ebx;
	cprintf("pid in sysproc.c is %d\n",pid);
 	return sort_procces(pid);
 }

 int sys_get_max_invoked_syscall(void){
	int pid = myproc()->tf->ebx;
	cprintf("pid in sysproc.c is %d\n",pid);
 	return get_max_invoked_syscall(pid);
 }
int sort_procces(int pid)
{
  struct proc *p;
  int i, j;
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if (p->pid == pid)
    {
   
      for (i = 0; i < p->numofsyscalls - 1; i++)
      {
        for (j = i + 1; j < p->numofsyscalls; j++)
        {
          if (p->syscalls[i] > p->syscalls[j])
          {
            
            int temp = p->syscalls[i];
            p->syscalls[i] = p->syscalls[j];
            p->syscalls[j] = temp;
          }
        }
      }
      for (i = 0; i < p->numofsyscalls; i++)
      { 
        cprintf("%d ", p->syscalls[i]);
      }
      cprintf("\n");

      return 0; 
    }
  }
  return -1;
}
int get_max_invoked_syscall(int pid)
{
  struct proc *p;
  int i, j;
  struct proc* target_p;
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if (p->pid == pid)
    {
      target_p=p;
   
      for (i = 0; i < p->numofsyscalls - 1; i++)
      {
        for (j = i + 1; j < p->numofsyscalls; j++)
        {
          if (p->syscalls[i] > p->syscalls[j])
          {
            
            int temp = p->syscalls[i];
            p->syscalls[i] = p->syscalls[j];
            p->syscalls[j] = temp;
          }
        }
      }
     int count[300];
     memset(count,0,300);
     for(int i=0;i<300;i++){
      count[i]=0;
     // cprintf("%d \n",count[i]);
      
     }
     
     for (int i=0;i<target_p->numofsyscalls;i++){
      count[target_p->syscalls[i]]++;
     }
     int max=-1;
     int max_index=-1;

     for(int i=0;i<30;i++){
     if(count[i]>=max && count[i]!=0){
      max=count[i];
      max_index=i;
     }
     
     }
     if(max==-1){
      cprintf("no syscall found \n");
      return -1;
     }
        for(int i=0;i<30;i++){
     if(count[i]==max){
       cprintf("num of the system call is %d and it invoked is %d \n",i,count[i]);
       return  i;
     }
     }
      // cprintf("num of the system call is %d and it invoked is %d \n",max_index,count[max_index]);
      return 0; 
    }
  }
  cprintf("Pid not found \n");
  return -1;
}
