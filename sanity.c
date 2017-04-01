#include "types.h"
#include "stat.h"
#include "perf.h"
#include "user.h"

struct ProcStats {
   int pid;
   char * type;
   int status;
   struct perf * performance; 
};

int main(int argc, char *argv[]) {
    //int stat;
    int totalProcess = 100;
    struct ProcStats *array[totalProcess];

    if(argc > 1){
        printf(1, "sanity %s not a valid argument: no arguments \n",argv[1]);
        exit(0);
    }

    for (int i = 0 ; i < totalProcess ; i++){
        int pid = fork();
        if (pid == 0) {
            array[i]->pid = pid;
            printf(1, "son %d \n", array[i]->pid);
            exit(0);
        }
    }
    exit(0);
}