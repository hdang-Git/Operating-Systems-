#include "utility.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void my_cd(char* args[]){

}

void my_clear(){
	printf("SUCCESS!\n");
	system("clear");
}

void my_dir(){
}

void my_env(){
}

void my_echo(char* args[]){
	int i;
	//printf("echo called, size of array passed: %ld\n", sizeof(args)/sizeof(args[0]));
	for(i = 1; args[i] != NULL; i++){
		printf("%s ", args[i]);
	}
	printf("\n");

}

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
