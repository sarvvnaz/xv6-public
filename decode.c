#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int
main(int argc, char *argv[])
{
  int i;
  for(i = 1; i < argc ; i++)
  {
  int s = strlen(argv[i]);
  int j;
  	for (j = 0; j < s ; j ++){
	    if ((argv[i][j] >= 'A' && argv[i][j] <= 'Z') || (argv[i][j] >= 'a' && argv[i][j] <= 'z'))
	    	{
	    		char base = (argv[i][j] >= 'A' && argv[i][j] <= 'Z') ? 'A' : 'a';
	    		argv[i][j] = (argv[i][j] - base + 15) %26 + base;
	    	}
	    }
  }
  int f ;
  unlink("result.txt");
  f = open("result.txt",O_CREATE|O_RDWR);
  int k;
  for(k = 1; k < argc; k++){
	  write(f,argv[k],strlen(argv[k]));
 	 write(f," ",1);}
  write(f,"\n",1);
  close(f);
  exit();
}
