#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{

  if(argc <= 1 || argc > 2){
    printf(1, "policy %s not a valid argument: \n 0 - Uniform time distribution \n 1 - Priority scheduling \n 2 - Dynamic tickets allocation \n",argv[1]);
    exit(0);
  }

  int i;
  i = atoi(argv[1]);
  if (i > 2 || i < 0){
    printf(1, "policy %s not a valid argument: \n 0 - Uniform time distribution \n 1 - Priority scheduling \n 2 - Dynamic tickets allocation \n",argv[1]);
    exit(0);
  }
  policy(i);

  exit(0);
}
