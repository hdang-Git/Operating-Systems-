/*
 * This program measures the time between forking and execution for two child processes iterated a hundred 
 * times. The differences in times between execution and forking is then recorded into an output 
 * file. A total of three processes will be created for each iteration one parent and two children.
 */

#define SIZE 100
#include<stdio.h>
#include<sys/time.h>
#include<unistd.h>
#include<stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

//function prototypes
void launch(char*, char*, pid_t, pid_t, int, int);
void launch2(char*, pid_t, int);
long int getTime(struct timeval);

/*********************************************************************************************
 * The main function calls the launch() function a hundred times which in turn  create a     *
 * child processes that executes the executable of Application.c                               *            
 *********************************************************************************************/
int main(){
    int i;
    char* fileName1 = "timer2_a.csv";			    //Set the filename for first child process
    char* fileName2 = "timer2_b.csv";			    //Set the filename for second child process
    FILE* output1 = fopen(fileName1, "w");		    //Create a file to write to for first child process
    FILE* output2 = fopen(fileName2, "w");		    //Create a file to write to for second child process
    pid_t pid1;							
    pid_t pid2;
    char* style = "***************************************"; 
    for(i = 0; i < SIZE; i++){
        printf("%s iteration %d %s\n", style, i, style);
	launch(fileName1, fileName2, pid1, pid2, 1, 2);     //Call launch() to create a process forked two times
    }
}

/*********************************************************************************************
 * This function gets the current time of day and returns the the time in microseconds       *
 *          										     *
 * Preconditions:                                                                            *
 * t - a struct timeval                                                                      *
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
 * Else if it is a parent process it calls launch2() to fork and create another child. It 
 * also waits for the first child process to finish. Error Handling is also created for      *
 * the case where the pid returns a negative value signaling an error value.                 *
 *                                                                                           *
 * Preconditions:                                                                            *
 * @params filename - output file name for first child process                               *
 * @params filename2 - output file name for second child process                             *
 * @params pid - process id for first fork                                                   *
 * @params pid2 - process id for second fork                                                 *
 * @params num - value to denote 1st child process                                           *
 * @params num2 - value to denote 2nd child process                                          *
 * @return void                                                                              *
 *********************************************************************************************/
void launch(char* filename, char* filename2, pid_t pid, pid_t pid2, int num, int num2){
    struct timeval t;
    long int start = getTime(t);				//get start time in microseconds
    char startTime[50];
    sprintf(startTime, "%ld", start);				//Convert start to char array
    printf("Start: %ld\n", start);

    pid = fork();						//fork the current process
           
    if(pid == 0){  //1st child process
        printf("Child %d process with id %d. My parent is %d. \n", num, getpid(),  getppid());
  	//Execute the executable file which will replace current child process
        execlp("./app", "./app", "outFile1.txt", startTime, filename, NULL); 	
        exit(0);
    } else if(pid < 0){  //Error handling 
        perror("fork error");                                   //Print error message
        exit(-1);                                               //Indicate unsuccessful program termination
    } else { //parent process
        printf("Parent process with id %d my child is %d. \n", getpid(),  pid);
	printf("Creating a new fork for child 2\n");
	launch2(filename2, pid2, 2);		       //Call launch2() to create second child process
        waitpid(pid, NULL, 0);                                 //Wait for first child process to finish
    }
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
 * @params num - value to denote second fork                                                 *
 * @return void                                                                              *
 *********************************************************************************************/
void launch2(char* filename, pid_t pid, int num){
    struct timeval t;
    long int start = getTime(t);				//get start time in microseconds
    char startTime[50];
    sprintf(startTime, "%ld", start);				//Convert start to char array
    printf("Start: %ld\n", start);
   
    pid = fork();						//fork the current process 
           
    if(pid == 0){  //child process
        printf("Child %d process with id %d. My parent is %d. \n", num, getpid(),  getppid());
	//Execute the executable file which will replace current child process
        execlp("./app", "./app", "outFile2.txt",startTime, filename, NULL); 		
        exit(0);
    } else if(pid < 0){  //Error handling 
        perror("fork error");					//Print error message
        exit(-1);						//Indicate unsuccessful program termination
    } else { //parent process
        printf("Parent process with id %d my child is %d. \n", getpid(),  pid);
        waitpid(pid, NULL, 0);					//Wait for second child process to finish
    }
}

