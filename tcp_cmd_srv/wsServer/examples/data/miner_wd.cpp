#include <stdio.h>
#include <iostream>
#include <string>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

using namespace std;

int is_run = 1;

void sig_handler(int signo)
{
    	if (signo == SIGUSR1)
       	printf("received SIGUSR1\n");
    	else if (signo == SIGKILL)
       	printf("received SIGKILL\n");
    	else if (signo == SIGSTOP)
   		printf("received SIGSTOP\n");
	else if (signo == SIGHUP)
        	printf("received SIGHUP\n");
	else if (signo == SIGINT)
        	printf("received SIGINT\n");
	else if (signo == SIGQUIT)
       	printf("received SIGQUIT\n");

	is_run = 0;
}

void checkProcess(char *process)
{
	FILE *read_fp;
	char check_cmd[128];
	char run_cmd[128];
    	char buffer[1024];
    	int chars_read;

	memset(check_cmd, '\0', sizeof(check_cmd));
    	memset(buffer, '\0', sizeof(buffer));

	sprintf(check_cmd,"ps -A | grep %s",process);

    	read_fp = popen(check_cmd, "r");
    	if(read_fp != NULL) {
       	chars_read = fread(buffer, sizeof(char), 1024, read_fp);
        	if (chars_read > 0) {
            		//printf("output was: - \n%s\n", buffer);
            		printf("%s Alive!\n",process);
        	} else {
			printf("%s not found\n",process);
			memset(run_cmd, '\0', sizeof(run_cmd));
			sprintf(run_cmd,"/root/%s &",process);
			system(run_cmd);
			printf("system call %s\n",run_cmd);
        	}
        	pclose(read_fp);
    	}
}

int main() 
{
   	time_t timep; 
	struct tm *p;

	if (signal(SIGUSR1, sig_handler) == SIG_ERR)
       	printf("\ncan't catch SIGUSR1\n");
    	if (signal(SIGKILL, sig_handler) == SIG_ERR)
       	printf("\ncan't catch SIGKILL\n");
    	if (signal(SIGSTOP, sig_handler) == SIG_ERR)
       	printf("\ncan't catch SIGSTOP\n");
	if (signal(SIGHUP, sig_handler) == SIG_ERR)
       	printf("\ncan't catch SIGHUP\n");
	if (signal(SIGINT, sig_handler) == SIG_ERR)
       	printf("\ncan't catch SIGINT\n");
	if (signal(SIGQUIT, sig_handler) == SIG_ERR)
       	printf("\ncan't catch SIGQUIT\n");

	printf("===========Start Miner WD===========\n");

	while(is_run) {
		time(&timep); 
		p=localtime(&timep); 

		if(p->tm_sec == 0) { //check process per min
			checkProcess("miner_info");
			checkProcess("data_server");
		}
		
		sleep(1);
	}

	printf("===========Leave Miner WD===========\n");
    	return 0;
}
