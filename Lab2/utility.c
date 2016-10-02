#include "utility.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>



void my_cd(char* args[], int size){
	if(size > 1){ 								//if more than 1 argument excluding 'cd'
		if(chdir(args[1]) != 0){
		printf("Error! %s\n", strerror(errno));
		} 
	} else {									//if no arguments 
	    char cwd[1024];
	    getcwd(cwd, sizeof(cwd));         		//get current working directory 
	    printf("MyShell~%s\n", cwd);  			
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
void my_help(char* args[], int size){
	
	printf("GNU bash, version 4.3.46(1)-release (x86_64-pc-linux-gnu) \n");
	printf("These shell commands are defined internally. Type `help' to see this list. \n");
	printf("Type `help name' to find out more about the function `name'.\n\n\n");
    int i;
    char* built_in[] = {"cd", "clr", "dir", "environ", "echo", "help", "pause", "quit", "\0"};
    //if only 'help' is typed with no other args (other args are null)
    if(size == 1){
		for(i = 0; strcmp(built_in[i], "\0") != 0; i++){
			printf("%s\n", built_in[i]);
		}
	}else if(strcmp(args[1], "cd") == 0){
		printf("Usage:\n cd [dir][options]\n");
		printf("Change the shell working directory. \n\n");
		printf("Change the current directory to DIR.  The default DIR is the value of the"
		" HOME shell variable. \n\n");
		printf("The variable CDPATH defines the search path for the directory containing"
		" DIR.  Alternative directory names in CDPATH are separated by a colon (:). A null "
		"directory name is the same as the current directory. If DIR begins with a slash "
		"(/), then CDPATH is not used.\n\n");
		printf("If the directory is not found, and the shell option `cdable_vars' is set, "
		"the word is assumed to be  a variable name.  If that variable has a value, its " 			" value	is used for DIR.\n\n");
	} else if(strcmp(args[1], "clr") == 0){
		printf("Usage:\n clr\n");
		printf("Clears the screen\n\n");
	} else if(strcmp(args[1], "dir") == 0){
		printf("Usage:\n dir [-clpv][+N][-N]\n");
		printf("Displays directory stack.\n\n");
		printf("Display the list of currently remembered directories.\n\n");
	} else if(strcmp(args[1], "environ") == 0){
		printf("Usage: \nenviron\n\n");
		printf("Displays system environment variables of current user.\n\n");
	} else if(strcmp(args[1], "echo") == 0){
		printf("Usage: \necho [-neE] [arg ...]\n\n");
		printf("Write arguments to the standard output.\n\n");
		printf("Display the ARGs, spearated by a single space character and followed by a "
		"newline, on the standard output.\n\n");
	} else if(strcmp(args[1], "help") == 0){
		printf("Usage:\n help [-dms][pattern ...]\n\n");
		printf("Display information about builtin commands.\n\n");
		printf("Display brief summaris of bultin commands. If PATTERN is specified, gives "
		"detailed help on all commands matching Pattern, otherwise the list of help topis is "
		"printed.\n\n");
	} else if(strcmp(args[1], "pause") == 0){
		printf("Usage:\n pause\n");
		printf("Freezes all running processes until ENTER key is pressed.\n\n");
	} else if(strcmp(args[1], "quit") == 0){
		printf("Usage: quit\n");
		printf("User exits the shell program, returning exit signal 0 to Unix system.\n\n");
	}  else {
		printf("help: no help topics match '%s'. Try 'help help'.\n", args[1]);
	}
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
