#include "types.h"
#include "stat.h"
#include "user.h"
#include "perf.h"

int main() {

  int pid = 0;
  int numOfThreads = 15;
  int n = 100;
  int i, factorial;
  struct perf perfs[numOfThreads];
  int stats[numOfThreads];
  int pids[numOfThreads];

  int ttime = 0;
  int retime = 0;
  int io_ttime = 0;
  int cpu_ttime = 0;
  int io_retime = 0;
  int cpu_retime = 0;

    printf(1, "\n*** Start sanity check ***\n\n");

      for(i = 0; i < numOfThreads; i++){
        pid = fork();
        if(pid == 0){
            if(i%2 == 0){ // CPU bound thread
                factorial = 0;
                for(i=1; i<=n; ++i)
                    factorial *= i;              
            }
            else if (i%2== 2){ // IO bound thread
                factorial = 0;
                for(i=1; i<=n; ++i){
                    factorial *= i; 
                    sleep(1); 
                }
            }
            exit(0);
            }
        }

      for(i = 0; i < numOfThreads; i++){
        pid = wait_stat(&stats[i], &perfs[i]);
        pids[i] = pid;
      }

      for(i = 0; i < numOfThreads; i++){
        if (i%2 == 0){ // CPU bound thread
            cpu_ttime += perfs[i].ttime - perfs[i].ctime;
            cpu_retime += perfs[i].retime;
        }
        if (i%2 == 1){ // IO bound thread
            io_ttime += perfs[i].ttime - perfs[i].ctime;
            io_retime += perfs[i].retime;
        }
        ttime += perfs[i].ttime - perfs[i].ctime;
        retime += perfs[i].retime;
        printf(1, "pid = %d\n", pids[i]);
        printf(1, "creation time = %d\n", perfs[i].ctime);
        printf(1, "termination time = %d\n", perfs[i].ttime);
        printf(1, "ready time = %d\n", perfs[i].retime);
        printf(1, "running time = %d\n", perfs[i].rutime);
        printf(1, "sleeping time = %d\n\n", perfs[i].stime);
      }

      printf(1, "\n    Turnaround Time = %d\n", ttime / numOfThreads);
      printf(1, "\n    Waiting Time = %d\n", retime / numOfThreads);
      printf(1, "\n    Turnaround Time for CPU bounded threads = %d\n", cpu_ttime / (numOfThreads / 2));
      printf(1, "\n    Waiting Time for CPU bounded threads = %d\n", cpu_retime / (numOfThreads / 2));
      printf(1, "\n    Turnaround Time for I/O bounded threads = %d\n", io_ttime / (numOfThreads / 2));
      printf(1, "\n    Waiting Time for I/O bounded threads = %d\n\n", io_retime / (numOfThreads / 2));
    }

/*

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
}*/