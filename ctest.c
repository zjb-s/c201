#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int main() {
    char * command;
    //FILE * fp;
    //fp = fopen("minicom", "w");
    while(1) {
        usleep(100000);
        //fprintf(command, "output[4].volts=10\n");
        system("echo 'output[4].volts=10\n' | minicom");
        usleep(100000);
        system("echo 'output[4].volts=0\n' | minicom");
    }
}