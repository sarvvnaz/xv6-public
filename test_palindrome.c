#include "types.h" 
#include "stat.h"
#include "user.h"

int
main(int argc, char* argv[]){
	if (argc!=2){
		if (argc < 2){
			printf(1,"number where?");
			}
		else if (argc>2){
			printf(1,"enough argumants");
			}
		exit();
		}
	else {
		int preEBX ;
		int num = atoi(argv[1]);
			printf(1,"USER: create_palindrome() is called for n = %d\n",num);
		}
		exit();
		}
