#define TRUE 1
#define FALSE 0
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
void parseCmd(char*, char*[]);
char* trim(char*);
int countSpace(char*,char);
char* check(char*, int*, int*, int*, int*, char**);

int main()
{
  char* inputCmd = NULL;
  int bytesRead; //# of bytes read from getline()
  //int argc;
  //char* argv[100];
  size_t size = 100;
  
  char cwd[1024];
  char* built_in[] = {"ls", "exit", "cd", "ls", "help"}; // list of commands
  //char* cmd;  
  int valid = FALSE;			//valid command
  int bg = FALSE;				//background process
  //int l_bracket = FALSE;		//stdin redirection
  //int r_bracket = FALSE;        //stdout redirection

  while(TRUE){
	  getcwd(cwd, sizeof(cwd));         //get current working directory 
      printf("MyShell~%s: ", cwd);  
      
      bytesRead = getline(&inputCmd, &size, stdin); //get user input and the error code
      if(bytesRead == -1){
          printf("Error! %d\n", bytesRead);
      } else {                                      //user input scanner successful
          printf("Before: %s\n", inputCmd);
          inputCmd = trim(inputCmd);
          printf("After: %s\n", inputCmd);
          int a = countSpace(inputCmd, ' ');
          char* cmd[a];
          printf("%d \n", a);
          parseCmd(inputCmd,cmd);
      }
   }
}

char* trim(char* input){
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
	while(length > 0 && !isalnum(*str)){
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


char* check(char* input, int* valid, int* bg, int* left, int* right, char** cmd){
	printf("valid: %d\n background: %d\n left %d\n right %d\n ", *valid, *bg, *left, *right);
    //if the first argment is in the list of commands
        //if second argument is ampersand
    // if anywhere there is a redirection for every odd argument (input/output)
    int i;
   
}


/*

http://stackoverflow.com/questions/298510/how-to-get-the-current-directory-in-a-c-program

*/
