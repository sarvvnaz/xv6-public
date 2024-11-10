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
		int num = atoi(argv[1]);
		asm volatile(
			"movl %%ebx, %0;"
			"movl %1, %%ebx;"
			: "=r" (preEBX)
			: "r"(num)
			);
		printf(1,"USER: create_palindrome() is called for n = %d\n",num);
		int answer = create_palindrome();
		printf(1,"palindrome of %d is %d\n",num,answer);
		asm(
			"movl %0, %%ebx"
			:
			: "r"(preEBX)
		);
		}
		exit();
		}
