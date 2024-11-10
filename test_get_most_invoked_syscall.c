#include "types.h" 
#include "stat.h"
#include "user.h"

int
main(int argc, char* argv[]){
	if (argc!=2){
		if (argc < 2){
			printf(1,"number where?\n");
			}
		else if (argc>2){
			printf(1,"too many argumants\n");
			}
		exit();
		}
	else {
		int preEBX ;
		int pid = atoi(argv[1]);
		int zae = 0;
		asm volatile(
			"movl %%ebx, %0;"
			"movl %1, %%ebx;"
			: "=r" (preEBX)
			: "r"(pid)
			);
		printf(1,"pid in test_sort_syscalls is:");
		printf(1,zae);
		printf(1,"\n");
		int answer = get_max_invoked_syscall();
		if (answer != 0){
			printf(1,"wrong pid or something!\n");
			}
		else{
			printf(1,"test completed\n");
			}
		asm(
			"movl %0, %%ebx"
			:
			: "r"(preEBX)
		);
		}
		exit();
		}
