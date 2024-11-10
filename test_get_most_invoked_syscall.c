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
		int pid = atoi(argv[1]);
		int answer = get_max_invoked_syscall(pid);
		if (answer != 0){
			printf(1,"wrong pid or something!\n");
			}
		else{
			printf(1,"test completed\n");
			}
		}
		exit();
		}
