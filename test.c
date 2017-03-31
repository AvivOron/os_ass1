#include "types.h"
#include "stat.h"
#include "user.h"

int main() {
    int status;
    for (int i = 0 ; i < 3 ; i++){
        policy(i);
        if (fork() >0) {
            wait(&status);
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