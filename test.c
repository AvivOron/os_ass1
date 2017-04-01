#include "types.h"
#include "stat.h"
#include "perf.h"
#include "user.h"

int main() {
    int status;
    struct perf* performance = 0;
    for (int i = 0 ; i < 3 ; i++){
        policy(i);
        if (fork() >0) {
            wait_stat(&status, performance);
            sleep(10);
            priority(status);
            if (status != 137) {
                printf(1, "Fail\n");
            } else {
                printf(1, "Passed\n");
            }
        } 
    }
    exit(137);
}