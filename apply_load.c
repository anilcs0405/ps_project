#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

void gen_random(char *s, const int len) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    int i = 0;
    for (i = 0; i < len; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    s[len] = 0;
}

int main(int argc, char *argv[])
{
    int sleep_intvl = 1;
    int port_no = 517113, temp_pno= 0;
    char fname[9];
    char command[50];
    
    srand(time(NULL));
    while(1){
	gen_random(fname, 8);
	temp_pno = port_no + (rand()%8);
	sprintf(command, "./client localhost %d %s",temp_pno, fname);
	system(command);
	//sleep(1);
    }
    return 0;
}
