/*
 * This program implements a shell with added features of batch processing at startup 
 * if a file is included, background processing, file i/o redirection, interprocess 
 * communication piping, along with the execution of builtins and non-builtins. 
 *
 *		Note:
 *			sample cmd-
 *				Ex.		./myshell
 *
 *			sample cmd with batch file
 *				Ex. 	./myshell batchfile
 */

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

/*********************************************************************************************
 * The main function takes in arguments and focuses on call functions that parse the input   *
 * batchfile, and store the list of arguments into an array created dynamically at runtime.  *
 * From there executeBatch() is called to execute the list of command arguments. After this  *
 * batch processing, the executeShell() function is called to handle all commands entered in *
 * manually by the user.																     * 
 *															                                 *
 * Preconditions:                                                                            *
 * Expects argv[1] to store a string filename i.e. "batchfile"                               *
 *********************************************************************************************/
int main(int argc, char* argv[])
{
   if(argc > 1){									
      char* batchName = argv[1];					//get the filename of the batch file
	  if(access(batchName, F_OK) == 0){				//if a valid file type
	 	 int lines = countLines(batchName);			//call countLines to return # of lines 
	 	 char* cmdList[lines];						//Create space in memory for commands 
	 	 readBatch(cmdList, batchName, lines); 		//Fill array with list of commands in file
	 	 int i;
	 	 for(i = 0; i < lines; i++){				//Loop array and execute individual cmd
			executeBatch(cmdList[i]);				
	 	 }
	  } else {
	 	 printf("Error with batch file: %s\n", strerror(errno));
	  }
   }
   executeShell();							//Call to func to executes commands with user input
}

/*********************************************************************************************
 * This function takes in a command in the form of a string and parses it into a new array   *
 * which stores the command followed by the following arguments. Then it calls other         *
 * functions that check if redirection or piping is called. In event of either, it does the  *
 * further necesssary parsing and then checks whether or not it is a builtin along with      *
 * whether or not it is a background process. If it is a builtin, it calls switchCmd() else  *
 * it is assumed to be a non-builtin which will be passed to launch(). After that is done,   *
 * settings are restored for redirection if it was called.                                   *
 *                                                                                           *
 * Preconditions:																			 *
 * @params inputCmd - a char pointer that points to a command string                         *
 * Postconditions:                                                                           *
 * @return void                                                                              *
 *********************************************************************************************/
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
  
    printPrompt();									//print current path in color
    if(verifyInput(inputCmd)){						//Do nothing
    } else {                                      	//user input scanner successful
        inputCmd = trim(inputCmd, ' ');				//get rid of leading & trailing spaces
        rmNewLine(inputCmd);						//get rid of '\n' character
        int size = countSpace(inputCmd, ' ');		//count the # of args delimited by spaces
        char* cmd[size];							//create array with size of # of args
        printf("size of input cmd array: %d \n", size);
        parseCmd(inputCmd,cmd);						//parse input into each index of array cmd
      	  
    	//Redirection Check
      	if(size > 2){ //at least 3 arguments total for redirection & pipe check
      		checkRedirect(cmd, size, &r, &rR, &l, &lL);				//check for Redirection
      	  	pipeIndex = checkPipe(cmd, size, &piping, &pipeCount);	//check for pipes
      	}
      	if(r || rR || l || lL){						//if any redirection symbols are used
      	 	//Retrieve output file name if > or >> are used
      	  	outputName = getFileRedir(cmd, size, outputName, &r, ">", &rR, ">>");
      	  	//Retrieve input file name if < or << are used
      	  	inputName = getFileRedir(cmd, size, outputName, &l, "<", &lL, "<<");
      	  	//Parse the cmd array removing the redirection symbols and reset the size
      	  	size = parseRedirect(cmd, size);	
      	}
      	//get and save the original file descriptors
      	getPrevFD(r, rR, l, lL, &saved_STDOUT, &saved_STDIN, outputName, inputName);

		//Execute either built_in (> -1) or non_builtins (== -1)
      	int index = check(cmd, size, &valid, &bg, built_in);
      	if(valid  && index > -1){					//valid built_in cmd
      	  	valid = FALSE;							//flip the flag to false for next read
      	  	switchCmd(index+1, cmd, size);			//call switch command to call builtin func
      	} else {									//not a built_in cmd
      	  	//fork the process and let the unix system handle it
      	  	launch(pid, cmd, &bg, &piping, pipeIndex, pipeCount, size);
      	}
      	//restore file descriptors and reset redirection operator flags
      	restore(saved_STDOUT, saved_STDIN, &r, &rR, &l, &lL); 
	}
}

/*********************************************************************************************
 * This function takes in user input in the form of a string and parses it into a new array  *
 * The command is stored in the array along with its arguments. Then it calls other          *
 * functions that check if redirection or piping is called. In event of either, it does the  *
 * further necesssary parsing and then checks whether or not it is a builtin along with      *
 * whether or not it is a background process. If it is a builtin, it calls switchCmd() else  *
 * it is assumed to be a non-builtin which will be passed to launch(). After that is done,   *
 * settings are restored for redirection if it was called.                                   *
 *                                                                                           *
 * Postconditions:                                                                           *
 * @return void                                                                              *
 *********************************************************************************************/
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
          inputCmd = trim(inputCmd, ' ');			//get rid of leading & trailing spaces
          rmNewLine(inputCmd);						//get rid of '\n' character
          int size = countSpace(inputCmd, ' ');		//count the # of args delimited by spaces
          char* cmd[size];							//create array with size of # of args
          parseCmd(inputCmd,cmd);					//parse input into each index of array cmd
      	  
      	  //Redirection Check
      	  if(size > 2){ //at least 3 arguments total for redirection & pipe check
      	  	checkRedirect(cmd, size, &r, &rR, &l, &lL);				//check for Redirection
      	  	pipeIndex = checkPipe(cmd, size, &piping, &pipeCount);	//check for pipes
      	  }
      	  if(r || rR || l || lL){					//if any redirection symbols are used
      	  	//Retrieve output file name if > or >> are used
      	  	outputName = getFileRedir(cmd, size, outputName, &r, ">", &rR, ">>");
      	  	//Retrieve input file name if < or << are used
      	  	inputName = getFileRedir(cmd, size, outputName, &l, "<", &lL, "<<");
      	  	//Parse the cmd array removing the redirection symbols and reset the size
      	  	size = parseRedirect(cmd, size);	
      	  }
      	  //get and save the original file descriptors
      	  getPrevFD(r, rR, l, lL, &saved_STDOUT, &saved_STDIN, outputName, inputName);

		  //Execute either built_in (> -1) or non_builtins (== -1)
      	  int index = check(cmd, size, &valid, &bg, built_in);
      	  if(valid  && index > -1){					//valid built_in cmd
      	  	valid = FALSE;							//flip the flag to false for next read
      	  	switchCmd(index+1, cmd, size);			//call switch command to call builtin func
      	  } else {									//not a built_in cmd
      	  	//fork the process and let the unix system handle it
      	  	launch(pid, cmd, &bg, &piping, pipeIndex, pipeCount, size);
      	  }
      	  //restore file descriptors and reset redirection operator flags
      	  restore(saved_STDOUT, saved_STDIN, &r, &rR, &l, &lL); 
      }
   }
}



/*********************************************************************************************
 * This function prints the hostname, logname, and current working directory in bold color.  * 
 *                                                                                           *
 * Postconditions:                                                                           *
 * @return void                                                                              *
 *********************************************************************************************/
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


/*********************************************************************************************
 * This function focuses on the batch file passed in and increments a counter to count the   *
 * number of lines in the files.                                                             *
 *                                                                                           *
 * Preconditions:                                                                            *
 * @params filename - name of the batch file to open                                         *
 * Postconditions:                                                                           *
 * @return the number of lines in the file                                                   *   
 *********************************************************************************************/
int countLines(char* filename){
	FILE* file;
	file = fopen(filename, "r");			//Set file mode to read
	int i = 0;								
	char line[128];							//Temporary array to store char array input
	while(fgets(line, sizeof(line), file) != NULL){		//read in line & cont. until it isn't NULL
		i++;								//Increment counter after each newline
	}
	fclose(file);							//Close filestream
	return i;
}


/*********************************************************************************************
 * This function takes in an array for the batch file and dynamically allocates space for    *
 * each argument. It then fills the array as it reads in each line including the '\n'        *
 * character.                                                                                *
 *                                                                                           *
 * Preconditions:                                                                            *
 * @params arr - the array to fill with the batch commands                                   *
 * @params filename - name of the batch file to open                                         *
 * @params numLines - number of lines in the file                                            *
 * Postconditions:                                                                           *
 * @return void                                                                              *
 *********************************************************************************************/
void readBatch(char* arr[], char* filename, int numLines){
	FILE* file;
	file = fopen(filename, "r");			//Set file mode to read
	char line[100];							//Temporary array to store line from file
	int i = 0;
	//Loop and read each line (including '\n') and stop when eof or NULL
	while(!feof(file) && fgets(line, sizeof(line), file) != NULL){
		printf("%s\n", line);
		arr[i] = malloc(strlen(line)+1);	//Allocate space in array index for each line
		arr[i] = strdup(line);				//Copy over char array to array
		i++;
	}
	fclose(file);							//Close filestream
}


/*********************************************************************************************
 * This function checks user input to see if the command entered is for instance a newline,  *
 * spaces, or characters that aren't alphanumeric and sets the flag to true.                 *
 *                                                                                           *
 * Preconditions:                                                                            *
 * @params cmd - the command entered into terminal                                           *
 * Postconditions:                                                                           *
 * @return flag of true or false                                                             *
 *********************************************************************************************/
int verifyInput(char* cmd){
	int flag = TRUE;			//flag up for \n, ' ', etc. 
	while(*(cmd) != '\0'){		
		if(isalpha(*(cmd))){	//if input contains any alphanumeric, set flag down
			flag = FALSE; 
		}
		*(cmd)++;
	}
	return flag;
}


/*********************************************************************************************
 * This function removes leading and trailing characters of the delimiter passed in,         *
 * specifically white spaces for this program.                                               *
 *                                                                                           *
 * Preconditions:                                                                            *
 * @params input - input string to change                                                    * 
 * @params delim - character of delimiter to look for and remove                             *
 * Postconditions:                                                                           *
 * @return formatted string without trailing/leading delimiters                              *
 *********************************************************************************************/
char* trim(char* input, char delim){
	//remove leading whitespace
	while(*input == delim){		     			//Move from front to back of string 
		input++;					 
	}								
	
	//remove trailing whitespace
	int length = strlen(input);
	char* str = input + length - 1;
	while(length > 0 && *input == delim){		//Move from back to front of string
		str--;
	}
	*(str+1) = '\0';							//Add a null character at end
	return input;
}


/*********************************************************************************************
 * This function removes any newlines in the command string as a result of file reading or   * 
 * the getline function that adds a newline when user presses ENTER key.                     *
 *                                                                                           *
 * Preconditions:                                                                            *
 * @params str - string command                                                              *
 * Postconditions:                                                                           *
 * @return void                                                                              *
 *********************************************************************************************/
void rmNewLine(char* str){
	while(*str != '\0'){
		if(*str == '\n')
			*str = '\0';
		str++;
	}
}


/*********************************************************************************************
 * This function counts the numbers of delimiters in the user entered command. For the scope *
 * of this program spaces are counted and                                                    *
 *                                                                                           *
 * Preconditions:                                                                            *
 * @params input - input string command                                                      *
 * @params delim - character of delimiter to look for                                        *
 * Postconditions:                                                                           *
 * @return number of spaces (arguments) in the string                                        *
 *********************************************************************************************/
int countSpace(char* input, char delim){
    int i;
    int count = 0;
	for(i = 0; input[i] != '\0'; i++)
		if(input[i] == delim)
			count++;
	return count + 1;
}

/*********************************************************************************************
 * This function parses the passed in command string and stores it into an array. Each       *
 * index of the array stores an argument which is separated by spaces in the string          *
 * Preconditions:                                                                            *
 * @params input - string command typed by user                                              *
 * @params arr - array of strings that will store parsed command                             * 
 * Postconditions:                                                                           *
 * @return void                                                                              *
 *********************************************************************************************/
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
   	   token = strtok(NULL, delimiter);	
   	   arr[i] = token;
   	   i++;	
   }
}


/*********************************************************************************************
 * This function checks if the passed string is a valid builtin or not. It also checks       *
 * whether or not it is a background process.                                                *
 *                                                                                           *
 * Preconditions:                                                                            *
 * @params input[] - stores command in first index with optional command options in rest     * 
 * @params size - size of the input[] array                                                  *
 * @params valid - flag that is true if it is a builtin, false otherwise                     *
 * @params bg - flag that is true if it is a background process, false otherwise             *
 * @params built_in[] - list of valid builtin commands                                       *
 * Postconditions:                                                                           *
 * @return valid flag that tells if it is a builtin or not                                   *
 *********************************************************************************************/
int check(char* input[], int size, int* valid, int* bg, char* built_in[]){
    int i;
    int index = -1;
    //Loop through builtins and compare against user input
    for(i = 0; strcmp(built_in[i],"\0") != 0; i++){
    	//if the first user argument is in the list of builtins
    	if(strcmp(input[0], built_in[i]) == 0){
    		*valid = TRUE;								//change flag to true for builtin
    		index = i;									//Retrieve index in built_in array 
    	}
    } 
    //if last argument is ampersand set background flag to true
    if(*(input[size-1]) == '&'){
    	*bg = TRUE;									//Set background flag to true
		input[size-1] = NULL; 						//Get rid of ampersand 
    } 
   return index;
}


/*********************************************************************************************
 * This function checks if any output or input redirection characters are in array storing   *
 * the command typed in by user.                                                             *
 *                                                                                           *
 * Preconditions:                                                                            *
 * @params args[] - stores command in first index with optional command options in rest      *
 * @params size - size of args[] array                                                       *
 * @params r  - flag for '>' character                                                       *
 * @params rR - flag for '>>' character set                                                  *
 * @params l  - flag for '<' character                                                       *
 * @params lL - flag for '<<' character set                                                  *
 * Postconditions:                                                                           *
 * @return void                                                                              *
 *********************************************************************************************/
void checkRedirect(char* args[], int size, int* r, int* rR, int* l, int* lL){
	int i;
	//Range is 1 to size-1 since redireciton chars can't be at beginning or end of string
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


/*********************************************************************************************
 * This function retrieves and returns the filename after the redirection operator, calling  *
 * getRedirectName() which parses the string.                                                *
 *                                                                                           *
 * Preconditions:                                                                            *
 * @params cmd[] - list of commands                                                          *
 * @params size - number of args in cmd[]                                                    *
 * @params fileName - name of the file after redirection operator                            *
 * @params a - flag of redirection operator  i.e. r, l                                       *
 * @params symbolA - redirection operator    i.e. "<", ">"                                   *
 * @params b - flag of redirection operator  i.e. rR, lL                                     *
 * @params symbolB - redirection operator    i.e. "<<", ">>"                                 *
 * Postconditions:                                                                           *
 * @return name of the file                                                                  *
 *********************************************************************************************/
char* getFileRedir(char* cmd[], int size, char* fileName, 
					int* a, char* symbolA, int* b, char* symbolB){
	if(a){
		fileName = getRedirectName(cmd, size, fileName, a, symbolA);
	}else if(b){
	  	fileName = getRedirectName(cmd, size, fileName, b, symbolB);
	}
	return fileName;
}


/*********************************************************************************************
 * This function parses the string in the passed array and gets the filename after the       * 
 * redirection operator.                                                                     *
 *                                                                                           *
 * Preconditions:                                                                            *
 * @params args[] - list of commands                                                         *
 * @params size - size of args[] array                                                       *
 * @params fileName - name of the file afeter redirection operator                           *
 * @params pointer - flag for redirection operator                                           *
 * @params delim - redirection operator to compare against                                   *
 * Postconditions:                                                                           *
 * @return name of file                                                                      *
 *********************************************************************************************/
char* getRedirectName(char* args[], int size, char* fileName, int* pointer, char* delim){
	int i;
	fileName = NULL;
	for(i = 1; i < size - 1; i++){				//first and last redirection operator !exist 
		if(strcmp(args[i], delim) == 0){		//match strings with operator
			fileName = args[i+1];  				//get filename after redirection operator
			*pointer = TRUE;					//set redirection flag to true
		} 
	}
	return fileName;
}

/*********************************************************************************************
 * This function loops through the passed array and if the current value is a redirection    *
 * operator, it NULLS everything at and past that point. It then returns the index of the    *
 * redirection operator.                                                                     *
 *                                                                                           *
 * Preconditions:                                                                            *
 * @params cmd - list of commands                                                            *
 * @params size - size of array cmd                                                          *
 * Postconditions:                                                                           *
 * @return index of redirection operator                                                     *
 *********************************************************************************************/
int parseRedirect(char* cmd[], int size){
	int i;
	int j = 0;
	char* arr[] = {"<", "<<", ">", ">>", "\0"};	//array of redirection operators
	for(i = 1; i < size - 1; i++){
		while(strcmp(arr[j], "\0") != 0){
			if(strcmp(cmd[i], arr[j]) == 0){
				cmd[i] = NULL;					//NULL out redirection operator
				return i;
			}
			j++;
		} 
	}
}


/*********************************************************************************************
 * This function retrieves the original file descriptors that were written over from calls   *
 * to writeToFile() and readFromFile() respectively which in turn replace the original       *
 * file descriptors.                                                                         *
 *                                                                                           *
 * Preconditions:                                                                            *
 * @params r  - flag for '>' character                                                       *
 * @params rR - flag for '>>' character set                                                  *
 * @params l  - flag for '<' character                                                       *
 * @params lL - flag for '<<' character set                                                  *
 * @params saved_STDOUT - stores original STDOUT file descriptor                             *
 * @params saved_STDIN - stores original STDIN file descriptor                               *
 * @params outputName - name of output file                                                  *
 * @params inputName  - name of input file                                                   *
 * Postconditions:                                                                           *
 * @return void                                                                              *
 *********************************************************************************************/
void getPrevFD(int r, int rR, int l, int lL, int* saved_STDOUT, int* saved_STDIN, 
				   char* outputName, char* inputName){
	//Open file and retrieve STDOUT
    if(r || rR){
   		*saved_STDOUT = writeToFile(outputName, r, rR);
    } 
    //Open file and retrieve STDIN
    if(l || lL){
     	*saved_STDIN= readFromFile(inputName, l, lL);
    }
}



/*********************************************************************************************
 * This function opens a file to write to. It then duplicates the file descriptor to write   *
 * to and deletes the old one.                                                               *
 *                                                                                           *
 * Preconditions:                                                                            *
 * @params fileName - name of file                                                           *
 * @params r  - flag for '>' redirection character                                           *
 * @params rR - flag for '>>' redirection character                                          *
 * Postconditions:                                                                           *
 * @return old file descriptor for STDOUT                                                    *
 *********************************************************************************************/
int writeToFile(char* fileName, int r, int rR){
	int fd;
	if(r){
		if((fd = open(fileName, O_WRONLY|O_CREAT|O_TRUNC, 0666)) == -1){ //create/trunc file
			printf("Error with '>' Redirection! %s\n", strerror(errno));
		}
    } else if(rR){
      	if((fd = open(fileName, O_WRONLY|O_CREAT|O_APPEND, 0666)) == -1){ //append file
      		printf("Error with '>>' Redirection! %s\n", strerror(errno));
      	}  	
    }
	int saved_STDOUT = dup(1);		//duplicate old file descriptor
	dup2(fd, STDOUT_FILENO);		//replace old file descriptor with newly open file
	return saved_STDOUT;
}


/*********************************************************************************************
 * This function opens a file to read from. It then duplicates the file descriptor to read   *
 * from and deletes the old one.                                                             *
 *                                                                                           *
 * Preconditions:                                                                            *
 * @params fileName - name of file                                                           *
 * @params l  - flag for '<' redirection character                                           *
 * @params lL - flag for '<<' redirection character                                          *
 * Postconditions:                                                                           *
 * @return old file descriptor for STDIN                                                     *
 *********************************************************************************************/
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
	int saved_STDIN = dup(0);	//duplicate old file descriptor
	dup2(fd, STDIN_FILENO);		//replace old file descriptor with newly open file
	return saved_STDIN;
}

/*********************************************************************************************
 * This function restores the old file descriptors after redirection is completed by calling *
 * restoreOutput() and restoreInput() functions respectively.                                *
 *                                                                                           *
 * Preconditions:                                                                            *
 * @params saved_STDOUT	- old STDOUT file descriptor                                         *
 * @params saved_STDIN  - old STDIN file descriptor                                          *
 * @params r  - flag for '>' redirection character                                           *
 * @params rR - flag for '>>' redirection character                                          *
 * @params l  - flag for '<' redirection character                                           *
 * @params lL - flag for '<<' redirection character                                          *
 * Postconditions:                                                                           *
 * @return void                                                                              *
 *********************************************************************************************/
void restore(int saved_STDOUT, int saved_STDIN, int* r, int* rR, int* l, int* lL){
	if(*r || *rR){
  		restoreOutput(saved_STDOUT);		//restore STD_OUT
  	}
   	if(*l || *lL){
      	restoreInput(saved_STDIN);			//restore STD_IN
    }
    //flip the redirection flags
    *r = FALSE;
    *rR = FALSE;
    *l = FALSE;
    *lL = FALSE;  
}


/*********************************************************************************************
 * This function restores STDOUT file descriptor.                                            *
 *                                                                                           *
 * Preconditions:                                                                            *
 * @params out - old STDOUT file descriptor                                                  *
 * Postconditions:                                                                           *
 * @return void                                                                              *
 *********************************************************************************************/
void restoreOutput(int out){
	dup2(out, 1);
	close(out);
}


/*********************************************************************************************
 * This function restores STDIN file descriptor.                                             *
 *                                                                                           *
 * Preconditions:                                                                            *
 * @params input - old STDIN file descriptor                                                 *
 * Postconditions:                                                                           *
 * @return void                                                                              *
 *********************************************************************************************/
void restoreInput(int input){
	dup2(input, 0);
	close(input);
}


/*********************************************************************************************
 * This function checks if there is a pipe in the passed array. If there is, it set the flag *
 * to true, increments a counter counting the number of pipes, and returns the index of the  *
 * pipe.                                                                                     *
 *                                                                                           *
 * Preconditions:                                                                            *
 * @params cmd[] - list of commands                                                          *
 * @params size  - size of cmd[] array                                                       *
 * @params pipe  - pipe flag, true if pipe exists, false otherwise                           *
 * @params count - number of pipes                                                           *
 * Postconditions:                                                                           *
 * @return index of encountered pipe                                                         *
 *********************************************************************************************/
int checkPipe(char* cmd[], int size, int* pipe, int* count){
	int i;
	int index = -1;
	*count = 0;
	//make sure first and last values in array aren't pipes
	if(strcmp(cmd[0], "|") != 0 || strcmp(cmd[size-1], "|") != 0){
		for(i = 1; i < size - 1; i++){
			if(strcmp(cmd[i], "|") == 0){
				*pipe = TRUE;					//set flag to true
				index = i;						//store index of pipe
				(*count)++;						//increment counter
			}
		}
	} else {
		printf("Error! \"|\" character cannot be at the beginning or end of cmd\n");
	}
	return index;
}

/*********************************************************************************************
 * This function creates a new array for one command and its options that can be executed    *
 * by copying over the original command without the pipe.                                    *
 *                                                                                           *
 * Preconditions:                                                                            *
 * @params cmd[] - list of commands                                                          *
 * @params arr[] - new array to copy to                                                      *
 * @params start - starting index of argument                                                *
 * @params end   - end index of argument right before the pipe                               *
 * @params name - name of array                                                              *
 * Postconditions:                                                                           *
 * @return size of newly created array                                                       *
 *********************************************************************************************/
int parsePipe(char* cmd[], char* arr[], int start, int end, char* name){
	int i;
	int j = 0;
	int size = end - start;										//calculate size of array
	for(i = start; i < end; i++){
		arr[j] = cmd[i]; 										//copy over string
		j++;
	}
	arr[j] = NULL;												//Null terminate array
	return size;
}


/*********************************************************************************************
 * This function uses a switch statement to switch between functions that implement the      *
 * chosen built_in to execute.                                                               *
 *                                                                                           *
 * Preconditions:                                                                            *
 * @params num - number matching the built_in function to call                               *
 * @params args[] - command list which is passed to builtins to access optional arguments    *
 * @params size - size of args[] to see if there are optional arguments passed in            *
 * Postconditions:                                                                           *
 * @return void                                                                              *
 *********************************************************************************************/
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


/*********************************************************************************************
 * This function executes the non-builtins by forking the current process and passing the    *
 * command array to exec() in the child process. It calls launchPipe if it pipes and it also *
 * handles background processes.                                                             * 
 *                                                                                           *
 * Preconditions:                                                                            *
 * @params pid - process id                                                                  *
 * @params args[] - list of commands                                                         *
 * @params bg - background flag                                                              *
 * @params piping - piping flag                                                              *
 * @params indexP - index of pipe character                                                  *
 * @params countP - number of pipe characters                                                *
 * @params size - size of args[] array                                                       *
 * Postconditions:                                                                           *
 * @return void                                                                              *
 *********************************************************************************************/
void launch(pid_t pid, char* args[], int* bg, int* piping, int indexP, int countP, int size){
	pid = fork();									//fork the current process
	int status;
	pid_t pid2;
	int pipefd[2];
	if(pid == 0){
		if(*piping){									//if it is a pipe
			launchPipe(pid2, pipefd, args, indexP, countP, size);	//call to pipe
		} else {
			execvp(args[0], args);					//else replace child with cmd with option args
		}
		exit(0);
	} else if(pid < 0){								//Error handling if error code
		perror("Error incorrect arguments\n");		//Print error message
        exit(-1);									//Indicate unsuccessful program termination
	} else {
		if(!(*bg)){									//if not background process, wait
			if(*piping){							//if piping
				if(wait(NULL) < 0){					//wait for all
					printf("Error with piping! %s\n", strerror(errno));
				}
				*piping = FALSE;					//Set pipe flag to false
			} else {
				if(waitpid(pid, &status, 0) < 0){	//wait for child process to finish
					perror("PID ERROR\n");
				} 
			}
		} else {									//if background, don't wait
			*bg = FALSE;
		}
	}
}


/*********************************************************************************************
 * This function loops through an array and prints out its contents.                         *
 *                                                                                           *
 * Preconditions:                                                                            *
 * @params arr[] - list of commands                                                          *
 * @params size - size of array                                                              *
 * @params str - name of array                                                               *
 * Postconditions:                                                                           *
 * @return void                                                                              *
 *********************************************************************************************/
void printArray(char* arr[], int size, char* str){
	int i;
	printf("%s size: %d\n", str, size);
	for(i = 0; i < size -1; i++){
		printf("%s : %s ", str, arr[i]);
	}
	printf("\n");
}

/*********************************************************************************************
 * This function handles the interprocess communication associated with piping by creating   *
 * two new arrays that parses the original command array. These new arrays are used with the *
 * parent writing to the pipe and the child reading from the pipe.                           *
 *                                                                                           *
 *                                                                                           *
 * Preconditions:                                                                            *
 * @params pid2 - process id for second fork                                                 *
 * @params pipefd[] - file descriptors for piping                                            *
 * @params args[] - list of commands                                                         *
 * @params index - index of pipe character                                                   *
 * @params countPipe - number of pipe characters                                             *
 * @param size of args[] array                                                               *
 * Postconditions:                                                                           *
 * @return void                                                                              *
 *********************************************************************************************/
void launchPipe(pid_t pid2, int pipefd[], char* args[], int index, int countPipe, int size){
	int status;
	char* output[index + 1];					//array that writes in pipe i.e. ls 
	char* input[size - (index+1) + 1];			//array that read in pipe   i.e. more
	int outSize = parsePipe(args, output, 0, index, "output");				
	int inSize = parsePipe(args, input, index+1, size, "input");			
	
	pipe(pipefd);
	pid2 = fork();
	if(pid2 == 0){
		dup2(pipefd[0], 0);						//copy over read
		close(pipefd[1]);
		execvp(input[0], input);				//takes in input and exec
		perror("Error piping for read end\n");
		exit(1); 
		
	} else if(pid2 < 0){
		perror("Error incorrect arguments\n");
		exit(-1);
	} else {
		dup2(pipefd[1], 1);		   				//copy over write 
		close(pipefd[0]);			
		execvp(output[0], output); 				//pushes out output
		perror("Error piping for write end\n");
		exit(1);
	}
}

