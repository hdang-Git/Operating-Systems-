//This program measures the time between forking and execution.

#include<stdio.h>
#include<sys/time.h>
#include<unistd.h>
#include<stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

#define SIZE 1
void launch(FILE*, char*, pid_t);
float getTime(struct timeval);

int main(){
    int i;
    char* fileName1 = "timer1.txt";
    FILE* output;
    output = fopen(fileName1, "w");
    pid_t pid1;
    char* style = "***************************************"; 
    for(i = 0; i < 5; i++){
        printf("%s iteration %d %s\n", style, i, style);
        launch(output, fileName1, pid1);
    }
}

float getTime(struct timeval t){
    gettimeofday(&t, NULL);
    return t.tv_usec;
}

void launch(FILE* output, char* filename, pid_t pid){
    struct timeval t;
    float start = getTime(t);
    float end = start;

    printf("Start: %f, End: %f\n", start, end);
   
    pid = fork();
           
    if(pid == 0){  //child process
        printf("Child process with id %d. My parent is %d. \n", getpid(),  getppid());
        end = getTime(t);
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

