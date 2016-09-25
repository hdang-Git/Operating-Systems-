#define TRUE 1
#define FALSE 0


#include <stdio.h>
#include <stdlib.h>
char* parseCommand(char*, int, int, int, char*);

int main()
{
  char* inputCmd = NULL;
  int bytesRead; //# of bytes read from getline()
  int argc;
  size_t size = 100;
  char* argv[100];
  
  char* cmd[] = {"cd", "ls", "help", "exit"}; // list of commands
  
  int valid = FALSE;			//valid command
  int bg = FALSE;				//background process
  int brackets = FALSE;			//<> thing 

  while(TRUE){
      printf("MYSHELL: ");
      bytesRead = getline(&inputCmd, &size, stdin);
      if(bytesRead == -1){
          printf("Error! %d\n", bytesRead);
      } else {
          printf("Success\n");
      }
   }
}

char* parseCommand(){
}
