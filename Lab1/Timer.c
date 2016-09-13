//This program measures the time between forking and execution.

#include<stdio.h>
#include<sys/time.h>
#include<unistd.h>
#include<stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

#define SIZE 1
void launch();
void writeToFile(char*);
int main(){
    char* fileName = "timer1.txt";
    //writeToFile(fileName);

    struct timeval t1;
    struct timeval t2;

    FILE* output;	
    output = fopen(fileName, "w");
    int i;

    gettimeofday(&t1, NULL);
    float start = t1.tv_usec;
    float end = start;
    for(i = 0; i < 5; i++){
	printf("Start: %f, End: %f\n", start, end);
	if(i > 0){
 	   gettimeofday(&t1, NULL);
	   start = t1.tv_usec;
        }
	pid_t pid = fork();
           

        if(pid == 0){  //child process
	    printf("Child process with id %d. My parent is %d. \n", getpid(),  getppid());
	    gettimeofday(&t2, NULL);
            end = t2.tv_usec;

            int diff = end - start;
            printf("\nEnd time of child : %f\n", end);
	    printf("TIME DIFF: %d\n", diff);   
	    fprintf(output, "%d\n", diff);
	    printf("Wrote to timer file\n");
	    fclose(output); 
	    execlp("./app", "./app", NULL); 
	    exit(0);
	 } else if(pid < 0){  //Error handling 
	    perror("fork error");
	    exit(-1);
	 } else { //parent process
	    printf("Parent process with id %d my child is %d. \n", getpid(),  pid);
	    waitpid(pid, NULL, 0);
	 }
    }
    //printf("TIME: %d\n", diff);   
}










/*
void writeToFile(char* fileName){
    FILE* output;
    printf("Filename: %s\n", fileName);
    fopen(fileName, "w"); 
    float time[SIZE];
    int i;
 
    
    printf("Start the clock\n");
    for(i = 0; i < SIZE; i++){
       printf("********************************** populating array %d **********************************\n\n", i);
       //time[i] = launch();
       //printf("\n\nTime : %f\n\n",time[i]);
       //fprintf(output, "%d", time[i]);
       launch();
    }
    //fwrite(time, sizeof(int), sizeof(time[SIZE]), output);

   // for(i = 0; i < SIZE; i++){
   //    printf("Time : %f \n", time[i]);
   // }

    fclose(output);
}
*/
/*
void launch(){

    struct timeval t1;
    struct timeval t2;
    float start, end = 0.0;
    gettimeofday(&t1, NULL);
    start =  t1.tv_usec;

    pid_t pid = fork();
    
    //printf("\nSTART TIME: %f\n", start);
    if(pid == 0){  //child process
        printf("Child process with id %ld\n", (long)getpid());
      
	gettimeofday(&t2, NULL);
        end = t2.tv_usec;
        printf("\nEND TIME: %f\n", end);

        //execlp("./app", "./app", NULL);
        exit(0);
    } else { //parent process
        printf("Parent process with id %ld\n", (long) getpid());
        waitpid(pid, NULL, 0);
    }   

    printf("\nDifference: %f\n", end - start);
    return end - start;
}
*/
