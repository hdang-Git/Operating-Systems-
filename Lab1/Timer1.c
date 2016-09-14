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
void launch(char*, pid_t);
long int getTime(struct timeval);


/*********************************************************************************************
 * The main function calls the launch() function a hundred times which in turn  create a     *
 * child process that executes the executable of Application.c                               *            
 *********************************************************************************************/
int main(){
    int i;
    char* fileName1 = "timer1.csv";				//Set the filename
    pid_t pid1;							//Create process id type 
    char* style = "***************************************"; 
    for(i = 0; i < SIZE; i++){
        printf("%s iteration %d %s\n", style, i, style);	
        launch(fileName1, pid1);        //Call launch() to run the routine of forking the process
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
 * @params filename - output file name                                                       *
 * @params pid - process id                                                                  *
 * @return void                                                                              *
 *********************************************************************************************/
void launch(char* filename, pid_t pid){
    struct timeval t;
    long int start = getTime(t);				//get start time in microseconds
    char startTime[50];
    sprintf(startTime, "%ld", start);				//Convert start to char array
    
    printf("Start: %ld\n", start);
    pid = fork();						//fork the current process
           
    if(pid == 0){  //child process
        printf("Child process with id %d. My parent is %d. \n", getpid(),  getppid());
        //Execute the executable file which will replace current child process
        execlp("./app", "./app", "output.txt", startTime, filename, NULL); 		
        exit(0);						
    } else if(pid < 0){  //Error handling if error code		
        perror("fork error");					//Print error message
        exit(-1);						//Indicate unsuccessful program termination
    } else { //parent process
        printf("Parent process with id %d my child is %d. \n", getpid(),  pid);
        waitpid(pid, NULL, 0);					//Wait for child process to finish
    }
}

