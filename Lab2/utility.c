#include "utility.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>



void my_cd(char* args[]){
	if(chdir(args[1]) != 0){
		printf("Error! %s\n", strerror(errno));
	} 
}

void my_clear(){
	printf("SUCCESS!\n");
	system("clear");
}

void my_dir(){
	DIR *dp = NULL;
	struct dirent *ep = NULL;
	if((dp = opendir("./")) == NULL){
		perror("Failure opening directory");
		exit(1);
	} else {
		while((ep = readdir(dp)) != NULL){
			printf("%s\n", ep->d_name);
		}
	}
	closedir(dp);
	
}

//TODO
void my_env(){
	extern char **environ;
	int i;
	for(i = 0; environ[i] != NULL; i++){
		printf("%s\n", environ[i]);
	}
}

void my_echo(char* args[]){
	int i;
	//printf("echo called, size of array passed: %ld\n", sizeof(args)/sizeof(args[0]));
	for(i = 1; args[i] != NULL; i++){
		printf("%s ", args[i]);
	}
	printf("\n");

}

//TODO
void my_help(){
}

void my_pause(){
	do 
	{
		printf("System Paused. Please press enter key to continue.\n");
	} while(getchar()!= '\n');
}

void my_quit(){
	exit(EXIT_SUCCESS);
}
