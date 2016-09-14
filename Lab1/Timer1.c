/*
 * This program measures the time between forking and execution for one process iterated a hundred 
 * times. The differences in times between execution and forking is then recorded into an output 
 * file. A total of two processes will be created for each iteration.
 */


#define SIZE 100	//Number of iterations of the forking execution routine

#include<stdio.h>
#include<sys/time.h>
#include<unistd.h>
#include<stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

//function prototypes
void launch(FILE*, char*, pid_t);
long int getTime(struct timeval);


/*********************************************************************************************
 * The main function calls the launch() function a hundred times which in turn  create a     *
 * child process that executes the executable of Application.c                               *            
 *********************************************************************************************/
int main(){
    int i;
    char* fileName1 = "timer1.csv";				//Set the filename
    FILE* output;						//Create a pointer to FILE
    output = fopen(fileName1, "w");				//Open a file to write into
    pid_t pid1;							//Create process id type 
    char* style = "***************************************"; 
    for(i = 0; i < SIZE; i++){
        printf("%s iteration %d %s\n", style, i, style);	
        launch(output, fileName1, pid1);			//Call launch() to run the routine of forking the process
    }
}


/*********************************************************************************************
 * This function gets the current time of day and returns the t5*he time in microseconds       *
 *          										     *
 * Preconditions:                                                                            *
 * t - a struct                                                                              *
 *                                                                                           *
 * Postconditions:                                                                           *
 * @return the time in microseconds as a long int                                            *
 *********************************************************************************************/
long int getTime(struct timeval t){
    gettimeofday(&t, NULL);
    return t.tv_sec * 1000000L + t.tv_usec;
}

/*********************************************************************************************
 * This function forks the current process and if the child is a process, executes the       *
 * the executable application, timing the difference in start & end time from fork to exec   *
 * Else if it is a parent process it waits for the child. Error Handling is also created for *
 * the case where the pid returns a negative value signaling an error value.                 *
 *                                                                                           *
 * Preconditions:                                                                            *
 * @params output - File pointer for output stream                                           *
 * @params filename - output file name                                                       *
 * @params pid - process id                                                                  *
 * @return void                                                                              *
 *********************************************************************************************/
void launch(FILE* output, char* filename, pid_t pid){
    struct timeval t;
    long int start = getTime(t);				//get start time in microseconds
    long int end = start;					//set end time to equal start time so difference is non-negative

    printf("Start: %ld, End: %ld\n", start, end);
   
    pid = fork();						//fork the current process
           
    if(pid == 0){  //child process
        printf("Child process with id %d. My parent is %d. \n", getpid(),  getppid());
        end = getTime(t);					//get the end time in microseconds
        long int diff = end - start;				//calculate the difference
        printf("\nEnd time of child : %ld\n", end);
        printf("TIME DIFF: %ld\n", diff);   
        fprintf(output, "%ld\n", diff);				//write to file the difference delimited by newlines
        printf("Wrote to timer file\n");
        fclose(output); 					//close the file I/O stream
        execlp("./app", "./app", "output.txt", NULL); 		//Execute the executable file which will replace current child process
        exit(0);						
    } else if(pid < 0){  //Error handling if error code		
        perror("fork error");					//Print error message
        exit(-1);						//Indicate unsuccessful program termination
    } else { //parent process
        printf("Parent process with id %d my child is %d. \n", getpid(),  pid);
        waitpid(pid, NULL, 0);					//Wait for child process to finish
    }
}

