#define TRUE 1
#define FALSE 0
#define _GNU_SOURCE
#define WHITE "\x1B[0m"
#define GREEN "\x1B[32m"
#define BLUE "\x1B[34m"
#define MAGENTA "\x1B[35m"
#define READ 0
#define WRITE 1


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include "utility.h"


//function prototypes
void printPrompt();
int verifyInput(char*);
int countLines(char*);
void readBatch(char*[], char*, int);
void executeBatch(char*);
void executeShell();
void parseCmd(char*, char*[]);
char* trim(char*, char);
int countSpace(char*,char);
void launch(pid_t, char*[], int*, int*, int, int, int);
int check(char*[], int, int*, int*, char*[]);
void rmNewLine(char*);
void switchCmd(int, char*[], int);
void checkRedirect(char*[], int, int*, int*, int*, int*);
char* getRedirectName(char*[], int, char*, int*, char*);
int writeToFile(char*, int, int);
int readFromFile(char*, int, int);
void getPrevFD(int, int, int, int, int*, int*, char*, char*);
void restoreOutput(int);
void restoreInput(int);
void cmdRedirect(char*[], int, int*, int*, int*, int*);
int parseRedirect(char*[], int);
void restore(int, int, int*, int*, int*, int*);
int checkPipe(char*[], int, int*, int*);
int parsePipe(char*[], char*[], int, int, char*);
void launchPipe(pid_t, int[], char*[], int, int, int);
void printArray(char*[], int, char*);
char* getFileRedir(char*[], int, char*, int*, char*, int*, char*);

int main(int argc, char* argv[])
{
   if(argc > 1){
  	  printf("batch file: %s\n", argv[1]);
      char* batchName = argv[1];
	  if(access(batchName, F_OK) == 0){		//if a valid file type
	 	 int lines = countLines(batchName);
	 	 char* cmdList[lines];
	 	 readBatch(cmdList, batchName, lines); 
	 	 int i;
	 	 for(i = 0; i < lines; i++){
			executeBatch(cmdList[i]);
	 	 }
	  } else {
	 	 printf("Error with batch file: %s\n", strerror(errno));
	  }
   }
   executeShell();
}

void executeBatch(char* inputCmd){
    //List of commands
    char* built_in[] = {"cd", "clr", "dir", "environ", "echo", "help", "pause", "quit", "\0"}; 
    int valid = FALSE;								//valid command
    int bg = FALSE;									//background process flag
    int piping = FALSE;								//piping flag
    char* outputName;								//output arg after >, >>
    char* inputName;									//input arg after <, <<
    int r = FALSE;        							//stdout redirection flag > 
    int rR = FALSE;									//stdout redirection flag >>
    int l  = FALSE;									//stdin  redirection flag <
    int lL = FALSE;									//stdin  redirection flag <<
    int saved_STDOUT;
    int saved_STDIN;
    int pipeIndex;									//stores index of first pipe
    int pipeCount;									//stores number of pipes it encounters
    pid_t pid;
  
    printPrompt();								//print current path in color
    if(verifyInput(inputCmd)){				//Do nothing
    } else {                                      //user input scanner successful
        inputCmd = trim(inputCmd, ' ');			//get rid of leading & trailing spaces
        rmNewLine(inputCmd);						//get rid of '\n' character
        int size = countSpace(inputCmd, ' ');		//count the # of args delimited by spaces
        char* cmd[size];							//create array with size of # of args
        printf("size of input cmd array: %d \n", size);
        parseCmd(inputCmd,cmd);					//parse input into each index of array cmd
      	  
    	//Redirection Check
      	if(size > 2){ //at least 3 arguments total for redirection & pipe check
      		checkRedirect(cmd, size, &r, &rR, &l, &lL);				//check for Redirection
      	  	pipeIndex = checkPipe(cmd, size, &piping, &pipeCount);	//check for pipes
      	  	printf("pipeIndex: %d   pipeCount: %d\n", pipeIndex, pipeCount);	
      	}
      	if(r || rR || l || lL){					//if any redirection symbols are used
      	 	//Retrieve output file name if > or >> are used
      	  	outputName = getFileRedir(cmd, size, outputName, &r, ">", &rR, ">>");
      	  	//Retrieve input file name if < or << are used
      	  	inputName = getFileRedir(cmd, size, outputName, &l, "<", &lL, "<<");
      	  	//Parse the cmd array removing the redirection symbols and reset the size
      	  	size = parseRedirect(cmd, size);	
       	 	printf("outputName: %s\tinputName %s\n", outputName, inputName);
      	}
      	//get and save the original file descriptors
      	getPrevFD(r, rR, l, lL, &saved_STDOUT, &saved_STDIN, outputName, inputName);
      	printf("pipe flag: %d\n", piping);
		//Execute either built_in (> -1) or non_builtins (== -1)
      	int index = check(cmd, size, &valid, &bg, built_in);
      	if(valid  && index > -1){					//valid built_in cmd
      		printf("I'm a VALID BUILT_IN CMD + index : %d\n", index);
      	  	valid = FALSE;							//flip the flag to false for next read
      	  	switchCmd(index+1, cmd, size);			//call switch command to call builtin func
      	} else {									//not a built_in cmd
      	  	printf("I'm an INVALID BUILT_IN CMD\n");
      	  	//fork the process and let the unix system handle it
      	  	launch(pid, cmd, &bg, &piping, pipeIndex, pipeCount, size);
      	}
      	//restore file descriptors and reset redirection operator flags
      	restore(saved_STDOUT, saved_STDIN, &r, &rR, &l, &lL); 
      	piping = FALSE;
	}
}


void executeShell(){
   char* inputCmd = NULL;
   int bytesRead; 									//# of bytes read from getline() func
   size_t inSize = 100;								//size of input from getLine() func
  
   //List of commands
   char* built_in[] = {"cd", "clr", "dir", "environ", "echo", "help", "pause", "quit", "\0"}; 
   int valid = FALSE;								//valid command
   int bg = FALSE;									//background process flag
   int piping = FALSE;								//piping flag
   char* outputName;								//output arg after >, >>
   char* inputName;									//input arg after <, <<
   int r = FALSE;        							//stdout redirection flag > 
   int rR = FALSE;									//stdout redirection flag >>
   int l  = FALSE;									//stdin  redirection flag <
   int lL = FALSE;									//stdin  redirection flag <<
   int saved_STDOUT;
   int saved_STDIN;
   int pipeIndex;									//stores index of first pipe
   int pipeCount;									//stores number of pipes it encounters
   pid_t pid;
   //Loop forever until a break in the program
   while(TRUE){
  	  printPrompt();								//print current path in color
      bytesRead = getline(&inputCmd,&inSize,stdin); //get user input and the error code
      if(bytesRead == -1){							//if Error reading, send a error msg to user
          printf("Error! %d\n", bytesRead);	
      } else if(verifyInput(inputCmd)){				//Do nothing
      } else {                                      //user input scanner successful
          //printf("Before trim: %s\n", inputCmd);
          inputCmd = trim(inputCmd, ' ');			//get rid of leading & trailing spaces
          rmNewLine(inputCmd);						//get rid of '\n' character
          //printf("After trim : %s\n", inputCmd);
          int size = countSpace(inputCmd, ' ');		//count the # of args delimited by spaces
          char* cmd[size];							//create array with size of # of args
          printf("size of input cmd array: %d \n", size);
          parseCmd(inputCmd,cmd);					//parse input into each index of array cmd
      	  
      	  //Redirection Check
      	  if(size > 2){ //at least 3 arguments total for redirection & pipe check
      	  	checkRedirect(cmd, size, &r, &rR, &l, &lL);				//check for Redirection
      	  	pipeIndex = checkPipe(cmd, size, &piping, &pipeCount);	//check for pipes
      	  	printf("pipeIndex: %d   pipeCount: %d\n", pipeIndex, pipeCount);	
      	  }
      	  if(r || rR || l || lL){					//if any redirection symbols are used
      	  	//Retrieve output file name if > or >> are used
      	  	outputName = getFileRedir(cmd, size, outputName, &r, ">", &rR, ">>");
      	  	//Retrieve input file name if < or << are used
      	  	inputName = getFileRedir(cmd, size, outputName, &l, "<", &lL, "<<");
      	  	//Parse the cmd array removing the redirection symbols and reset the size
      	  	size = parseRedirect(cmd, size);	
       	 	printf("outputName: %s\tinputName %s\n", outputName, inputName);
      	  }
      	  //get and save the original file descriptors
      	  getPrevFD(r, rR, l, lL, &saved_STDOUT, &saved_STDIN, outputName, inputName);
      	  printf("pipe flag: %d\n", piping);
		  //Execute either built_in (> -1) or non_builtins (== -1)
      	  int index = check(cmd, size, &valid, &bg, built_in);
      	  if(valid  && index > -1){					//valid built_in cmd
      	  	printf("I'm a VALID BUILT_IN CMD + index : %d\n", index);
      	  	valid = FALSE;							//flip the flag to false for next read
      	  	switchCmd(index+1, cmd, size);			//call switch command to call builtin func
      	  } else {									//not a built_in cmd
      	  	printf("I'm an INVALID BUILT_IN CMD\n");
      	  	//fork the process and let the unix system handle it
      	  	launch(pid, cmd, &bg, &piping, pipeIndex, pipeCount, size);
      	  }
      	  //restore file descriptors and reset redirection operator flags
      	  restore(saved_STDOUT, saved_STDIN, &r, &rR, &l, &lL); 
      	  piping = FALSE;
      }
   }
}

void printPrompt(){
	char hostName[1024];
	gethostname(hostName, sizeof(hostName));		//get host name
	char cwd[1024];
	getcwd(cwd, sizeof(cwd));         				//get current working directory 
	printf(GREEN);
	printf("\e[1m%s@%s\e[0m", getenv("LOGNAME"), hostName); //print login and host name in bold
	printf(MAGENTA);
    printf("\e[1m MyShell~%s:\e[0m ", cwd);  		//print directory in bold 
    printf(WHITE);
}

int countLines(char* filename){
	FILE* file;
	file = fopen(filename, "r");
	int i = 0;
	char line[128];
	while(fgets(line, sizeof(line), file) != NULL){
		i++;
	}
	printf("# of lines %d\n", i);
	fclose(file);
	return i;
}

void readBatch(char* arr[], char* filename, int numLines){
	FILE* file;
	file = fopen(filename, "r");
	char line[100];
	int i = 0;
	while(!feof(file) && fgets(line, sizeof(line), file) != NULL){
		printf("%s\n", line);
		arr[i] = malloc(strlen(line)+1);
		//strcpy(arr[i], line);
		arr[i] = strdup(line);
		i++;
	}
	
	int j;
	for(j = 0; j < numLines; j++){
		printf("%s", arr[j]);
	}
	fclose(file);
}

//checks if contains delimiters or not 
int verifyInput(char* cmd){

	int flag = TRUE;			//flag up for \n, ' ', etc. 
	while(*(cmd) != '\0'){
		if(isalpha(*(cmd))){	//if input contains alphanumeric, set flag down
			flag = FALSE; 
		}
		*(cmd)++;
	}
	return flag;
}

char* trim(char* input, char delim){
	//printf("initial length: %d\n", (int)strlen(input));
	//remove leading whitespace
	while(*input == delim){		     
		input++;					//change !isalnum(*input) to while *input == " " 
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
   return index;
}


void checkRedirect(char* args[], int size, int* r, int* rR, int* l, int* lL){
	int i;
	for(i = 1; i < size - 1; i++){
		if(strcmp(args[i], ">") == 0){
			*r = TRUE;
		} else if(strcmp(args[i], ">>") == 0){
			*rR = TRUE;
		} else if(strcmp(args[i], "<") == 0){
			*l = TRUE;
		} else if(strcmp(args[i], "<<") == 0){
			*lL = TRUE;
		}
	}
}


char* getFileRedir(char* cmd[], int size, char* fileName, 
					int* a, char* symbolA, int* b, char* symbolB){
	if(a){
		fileName = getRedirectName(cmd, size, fileName, a, symbolA);
	}else if(b){
	  	fileName = getRedirectName(cmd, size, fileName, b, symbolB);
	}
	return fileName;
}

char* getRedirectName(char* args[], int size, char* fileName, int* pointer, char* delim){
	int i;
	fileName = NULL;
	for(i = 1; i < size - 1; i++){
		if(strcmp(args[i], delim) == 0){
			printf("Contains %s\n", delim);
			fileName = args[i+1];  
			*pointer = TRUE;
			if(strcmp(delim, ">") == 0 || strcmp(delim, ">>") == 0) 
				printf("Outputting to: %s\n", fileName);
			else {
				printf("Reading from: %s\n", fileName);
			}
		} 
	}
	return fileName;
}

int parseRedirect(char* cmd[], int size){
	printf("In parseRedirect\n");
	int i;
	int j = 0;
	char* arr[] = {"<", "<<", ">", ">>", "\0"};
	for(i = 1; i < size - 1; i++){
		printf("CMD: %s\n", cmd[i]);
		while(strcmp(arr[j], "\0") != 0){
			printf("i: %d j: %d diff: %d\n", i, j, strcmp(cmd[i],arr[j]));
			if(strcmp(cmd[i], arr[j]) == 0){
				cmd[i] = NULL;	
				return i;
			}
			j++;
		} 
	}
	printf("cmd[0] = %s***\n", cmd[0]);
	printf("Success parsing redirect\n");
}


void getPrevFD(int r, int rR, int l, int lL, int* saved_STDOUT, int* saved_STDIN, 
				   char* outputName, char* inputName){
	//Open file and retrieve STDOUT
    if(r || rR){
   		*saved_STDOUT = writeToFile(outputName, r, rR);
    	printf("output File Name %s\n", outputName);
    } 
    //Open file and retrieve STDIN
    if(l || lL){
     	*saved_STDIN= readFromFile(inputName, l, lL);
      	printf("input File Name %s\n", inputName);	  	
    }
}

int writeToFile(char* fileName, int r, int rR){
	int fd;
	if(r){
		if((fd = open(fileName, O_WRONLY|O_CREAT|O_TRUNC, 0666)) == -1){
			printf("Error with '>' Redirection! %s\n", strerror(errno));
		}
    } else if(rR){
      	if((fd = open(fileName, O_WRONLY|O_CREAT|O_APPEND, 0666)) == -1){
      		printf("Error with '>>' Redirection! %s\n", strerror(errno));
      	}  	
    }
	int saved_STDOUT = dup(1);
	dup2(fd, STDOUT_FILENO);
	return saved_STDOUT;
}

int readFromFile(char* fileName, int l, int lL){
	int fd;
	if(l){
		if((fd = open(fileName, O_RDONLY, 0666)) == -1){
			printf("Error with '<' Redirection! %s\n", strerror(errno));
		}
	} else if(lL){
		if((fd = open(fileName, O_RDONLY, 0666)) == -1){
			printf("Error with '<<' Redirection! %s\n", strerror(errno));
		}
	}
	int saved_STDIN = dup(0);
	dup2(fd, STDIN_FILENO);
	return saved_STDIN;
}

void restore(int saved_STDOUT, int saved_STDIN, int* r, int* rR, int* l, int* lL){
	if(*r || *rR){
  		restoreOutput(saved_STDOUT);		//restore STD_OUT
  		printf("r: %d  rR: %d\n", *r, *rR);
  		printf("OUTPUT RESTORED!!!\n");
  	}
   	if(*l || *lL){
      	restoreInput(saved_STDIN);		//restore STD_IN
      	printf("l: %d  lL: %d\n", *l, *lL);
     	printf("INPUT RESTORED!!!\n");
    }
    //flip the redirection flags
    *r = FALSE;
    *rR = FALSE;
    *l = FALSE;
    *lL = FALSE;  
}

void restoreOutput(int out){
	dup2(out, 1);
	close(out);
}

void restoreInput(int input){
	dup2(input, 0);
	close(input);
}

int checkPipe(char* cmd[], int size, int* pipe, int* count){
	int i;
	int index = -1;
	*count = 0;
	if(strcmp(cmd[0], "|") != 0 || strcmp(cmd[size-1], "|") != 0){
		for(i = 1; i < size - 1; i++){
			if(strcmp(cmd[i], "|") == 0){
				*pipe = TRUE;
				index = i;
				(*count)++;
			}
		}
	} else {
		printf("Error! \"|\" character cannot be at the beginning or end of cmd\n");
	}
	return index;
}

int parsePipe(char* cmd[], char* arr[], int start, int end, char* name){
	int i;
	int j = 0;
	int size = end - start;
	for(i = start; i < end; i++){
		arr[j] = cmd[i]; 
		printf("Copying arr: %s %d %s\n", name, j, arr[j]);
		j++;
	}
	arr[j] = NULL;
	return size;
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

void launch(pid_t pid, char* args[], int* bg, int* piping, int indexP, int countP, int size){
	//pid_t wpid;
	printf("BACKGROUND STATUS: %d\n", *bg);
	pid = fork();									//fork the current process
	int status;
	pid_t pid2;
	int pipefd[2];
	if(pid == 0){
		//printf("Child process with id %d. My parent is %d. \n", getpid(),  getppid());
		if(*piping){									//if it is a pipe
			printf("I'm piping! pipeflag: %d\n", *piping);
			//*piping = FALSE;
			launchPipe(pid2, pipefd, args, indexP, countP, size);
		} else {
			if(*bg)									//if background, pass only one arg, &
				execlp(args[0], *args, NULL);
			printf("***Built_in: %s\n", args[0]);
			execvp(args[0], args);					//else replace child with cmd with option args
		}
		exit(0);
	} else if(pid < 0){								//Error handling if error code
		perror("Error incorrect arguments\n");		//Print error message
        exit(-1);									//Indicate unsuccessful program termination
	} else {
	    //printf("Parent process with id %d my child is %d. \n", getpid(),  pid);
		if(!(*bg)){									//if not background process, wait
			printf("Not a background process\n");
			//waitpid(pid, NULL, 0);				//Wait for child process to finish
			if(*piping){
				printf("Parent WAITING! Piping: %d\n", *piping);

				if(wait(NULL) < 0){
					printf("Error with piping! %s\n", strerror(errno));
				}
				*piping = FALSE;
			} else {
				if(waitpid(pid, &status, 0) < 0){
					perror("PID ERROR\n");
				} 
			}
		} else {									//if background, don't wait
			printf("I am a background process\n");
			*bg = FALSE;
		}
	}
}

void printArray(char* arr[], int size, char* str){
	int i;
	printf("%s size: %d\n", str, size);
	for(i = 0; i < size -1; i++){
		printf("%s : %s ", str, arr[i]);
	}
	printf("\n");
}
//TODO: create a new method so piping doesn't have to be start at 1st argument
void launchPipe(pid_t pid2, int pipefd[], char* args[], int index, int countPipe, int size){
	printf("in launchPipe() - index : %d    counter: %d    size: %d\n", index, countPipe, size); 
	int status;
	char* output[index + 1];				//out i.e. ls 
	char* input[size - (index+1) + 1];		//in  i.e. more
	printf("output pipe processing with size: %d\n", index);
	int outSize = parsePipe(args, output, 0, index, "output");
	printf("input pipe processing with size: %d\n", (size-(index+1)));
	int inSize = parsePipe(args, input, index+1, size, "input");
	printArray(output, outSize, "output");
	printArray(input, inSize, "input");
	
	pipe(pipefd);
	pid2 = fork();
	if(pid2 == 0){
		//close(WRITE); 			//close write end of pipe (free fd 1)
		printf("Child is reading from pipe\n");		
		dup2(pipefd[0], 0);			//copy over read
		close(pipefd[1]);
		execvp(input[0], input);	//takes in input and exec
		perror("Error piping for read end\n");
		exit(1); 
		
	} else if(pid2 < 0){
		perror("Error incorrect arguments\n");
		exit(-1);
	} else {
		//close read end and open write
		//close(0);
		printf("Writing to pipe!\n");
		dup2(pipefd[1], 1);		   
		close(pipefd[0]);
		execvp(output[0], output); //pushes out output
		perror("Error piping for write end\n");
		exit(1);
	}
}

