//This program measures the time between forking and execution.

#include<stdio.h>
#include<sys/time.h>
#include<unistd.h>
#include<stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

#define SIZE 1
void launch(FILE*, FILE*, char*, char*, pid_t, pid_t, int, int);
void launch2(FILE*, char*, pid_t, int);
float getTime(struct timeval);

int main(){
    int i;
    char* fileName1 = "timer2_a.txt";
    char* fileName2 = "timer2_b.txt";
    FILE* output1 = fopen(fileName1, "w");
    FILE* output2 = fopen(fileName2, "w");
    pid_t pid1;
    pid_t pid2;
    char* style = "***************************************"; 
    for(i = 0; i < 1; i++){
        printf("%s iteration %d %s\n", style, i, style);
        //launch(output1, fileName1, pid1, 1);
	launch(output1, output2, fileName1, fileName2, pid1, pid2, 1, 2);
    }
}

float getTime(struct timeval t){
    gettimeofday(&t, NULL);
    return t.tv_usec;
}

void launch(FILE* output, FILE* output2, char* filename, char* filename2, 
            pid_t pid, pid_t pid2, int num, int num2){
    struct timeval t;
    float start = getTime(t);
    float end = start;

    printf("Start: %f, End: %f\n", start, end);
   
    pid = fork();
           
    if(pid == 0){  //child process
        printf("Child %d process with id %d. My parent is %d. \n", num, getpid(),  getppid());
        end = getTime(t);
        int diff = end - start;
        printf("\nEnd time of child %d : %f\n", num, end);
        printf("TIME DIFF of child %d : %d\n", num, diff);
	//Write to file   
        fprintf(output, "%d\n", diff);
        printf("Wrote to timer of child %d file\n", num);
        fclose(output); 
        execlp("./app", "./app", NULL); 
        exit(0);
    } else if(pid < 0){  //Error handling 
        perror("fork error");
        exit(-1);
    } else { //parent process
        printf("Parent process with id %d my child is %d. \n", getpid(),  pid);
	printf("Creating a new fork for child 2\n");
	launch2(output2, filename2, pid2, 2);
        waitpid(pid, NULL, 0);
    }
}

void launch2(FILE* output, char* filename, pid_t pid, int num){
    struct timeval t;
    float start = getTime(t);
    float end = start;

    printf("Start: %f, End: %f\n", start, end);
   
    pid = fork();
           
    if(pid == 0){  //child process
        printf("Child %d process with id %d. My parent is %d. \n", num, getpid(),  getppid());
        end = getTime(t);
        int diff = end - start;
        printf("\nEnd time of child %d : %f\n", num, end);
        printf("TIME DIFF of child %d : %d\n", num, diff);
	//Write to file   
        fprintf(output, "%d\n", diff);
        printf("Wrote to timer of child %d file\n", num);
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

