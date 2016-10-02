#define TRUE 1
#define FALSE 0
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "utility.h"

//function prototypes
void parseCmd(char*, char*[]);
char* trim(char*, char);
int countSpace(char*,char);
void launch(pid_t, char*[], int*);
int check(char*[], int, int*, int*, char*[]);
void rmNewLine(char*);
void switchCmd(int, char*[], int);

int main()
{
  char* inputCmd = NULL;
  int bytesRead; //# of bytes read from getline()
  size_t size = 100;
  
  char cwd[1024];
  //List of commands
  char* built_in[] = {"cd", "clr", "dir", "environ", "echo", "help", "pause", "quit", "\0"}; 
  int valid = FALSE;								//valid command
  int bg = FALSE;									//background process
  //int l_bracket = FALSE;							//stdin redirection
  //int r_bracket = FALSE;        					//stdout redirection
  pid_t pid;
  while(TRUE){
	  getcwd(cwd, sizeof(cwd));         			//get current working directory 
      printf("MyShell~%s: ", cwd);  
      
      bytesRead = getline(&inputCmd, &size, stdin); //get user input and the error code
      if(bytesRead == -1){
          printf("Error! %d\n", bytesRead);	
      } else {                                      //user input scanner successful
          //printf("Before trim: %s\n", inputCmd);
          inputCmd = trim(inputCmd, ' ');
          rmNewLine(inputCmd);
          //printf("After trim : %s\n", inputCmd);
          int a = countSpace(inputCmd, ' ');
          char* cmd[a];
          printf("size of input cmd array%d \n", a);
          parseCmd(inputCmd,cmd);
      	  
      	  int index = check(cmd, a, &valid, &bg, built_in);
      	  if(valid  && index > -1){					//valid built_in cmd
      	  	printf("I'm a VALID BUILT_IN CMD + index : %d\n", index);
      	  	valid = FALSE;							//flip the flag to false for next read
      	  	switchCmd(index+1, cmd, a);				//call switch command
      	  } else {									//not a built_in cmd
      	  	printf("I'm a INVALID BUILT_IN CMD\n");
      	  	launch(pid, cmd, &bg);
      	  	
      	  	//fork the process and let the unix system handle it
      	  }
      	     
      }
   }
}

char* trim(char* input, char delim){
	//printf("initial length: %d\n", (int)strlen(input));
	//remove leading whitespace
	while(!isalnum(*input)){		//while first character is alphanumeric     
		input++;					//TODO: change isalnum() to while *input == " " 
	}								//      if cmd args aren't alphanumeric
	//printf("output: %s\n", input);
	
	//remove trailing whitespace
	int length = strlen(input);
	//printf("length: %d\n", length);
	char* str = input + length - 1;
	while(length > 0 && *input == delim){
		str--;
	}
	*(str+1) = '\0';
	//printf("post length: %d\n", (int)strlen(input));
	return input;
}

int countSpace(char* input, char delim){
    int i;
    int count = 0;
	for(i = 0; input[i] != '\0'; i++)
		if(input[i] == delim)
			count++;
	return count + 1;
}

void parseCmd(char* input, char* arr[]){
   char* token;
   const char delimiter[2] = " ";
   int i = 0;
   //Get the first token turning the first space to a Null character
   token = strtok(input, delimiter);
   arr[i] = token;
   i++;
   //Loop through all tokens with a space and aren't null
   while(token != NULL){
   	   printf("%s\t", token);	
   	   token = strtok(NULL, delimiter);	
   	   arr[i] = token;
   	   i++;	
   }
   printf("\n");
}

void rmNewLine(char* str){
	while(*str != '\0'){
		if(*str == '\n')
			*str = '\0';
		str++;
	}
}

//NOTE: Behavior of strcmp(input[0], built_in[i]) == 0) 
//the newline is included in input[0] with getline where comparison is
// "cd\n" == "cd" fails because newline
// "cd " == "cd" works, adding space pads away the \n into separate argument
int check(char* input[], int size, int* valid, int* bg, char* built_in[]){
	printf("valid: %d\n background: %d\n", *valid, *bg);
	printf("input cmd: %s\n", input[0]);
    //if the first argument is in the list of commands
    int i;
    int index = -1;
    for(i = 0; strcmp(built_in[i],"\0") != 0; i++){
    	//printf("i'm in the loop: %s\n", built_in[i]);
    	if(strcmp(input[0], built_in[i]) == 0){
    		*valid = TRUE;
    		index = i;
    		printf("condition is met\n");
    	}
    	//printf("difference comparator: %d\n", strcmp(input[0], built_in[i]));
    } 
    printf("valid %d\n", *valid);
    //if last argument is ampersand
    if(*(input[size-1]) == '&'){
    	*bg = TRUE;
    } 
    //printf("last character: %s\n", input[size-1]);
    //printf("background: %d\n", *bg);
    // if anywhere there is a redirection for every odd argument (input/output)
    
   return index;
}

void switchCmd(int num, char* args[], int size){
	switch(num){
		case 1	: 	my_cd(args, size);
					break;
		case 2	:	my_clear();	
					break;
		case 3	:	my_dir();
					break;
		case 4	:	my_env();
					break;
		case 5 	:	my_echo(args);
					break;
		case 6 	:	my_help(args, size);
					break;
		case 7	: 	my_pause();
					break;
		case 8	:	my_quit();
					break;
		default :	printf("Command DNE\n");
					break;
	}
}

void launch(pid_t pid, char* args[], int* bg){
	//pid_t wpid;
	printf("BACKGROUND STATUS: %d\n", *bg);
	pid = fork();									//fork the current process
	int status;
	if(pid == 0){
		//printf("Child process with id %d. My parent is %d. \n", getpid(),  getppid());
		if(*bg)										//if background, pass only one arg, &
			execlp(args[0], *args, NULL);
		execvp(args[0], args);						//else replace child with cmd with option args
		exit(0);
	} else if(pid < 0){								//Error handling if error code
		perror("Error incorrect arguments\n");		//Print error message
        exit(-1);									//Indicate unsuccessful program termination
	}else {
	    //printf("Parent process with id %d my child is %d. \n", getpid(),  pid);
		//if not background process, wait
		if(!(*bg)){
			printf("Not a background process\n");
			//waitpid(pid, NULL, 0);		//Wait for child process to finish
			
			
			if(waitpid(pid, &status, 0) < 0){
				perror("PID ERROR\n");
			}
			
			/*
			do{
				wpid = waitpid(pid, &status, WUNTRACED);
			} while(!WIFEXITED(status) && !WIFSIGNALED(status));
			*/
		} else {									//if background, don't wait
			printf("I am a background process\n");
			*bg = FALSE;
		}
	}
}

