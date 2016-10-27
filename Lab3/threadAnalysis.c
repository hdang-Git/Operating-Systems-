#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define NUMTHREADS 9
#define SIGTHREADS 3
#define HANDLETHREADS 2
#define SIGCOUNT 10

#define WHITE "\x1B[0m"
#define GREEN "\x1B[32m"
#define BLUE "\x1B[34m"
#define MAGENTA "\x1B[35m"
#define RED "\e[41m"

void* sig_handler();
void* handler1();
void* handler2();
float randNum(float, float);
int randBinary();
void signalUser1();
void signalUser2();

sigset_t mask;
pthread_t tid1[HANDLETHREADS];			//Handling threads for SIGUSR1
pthread_t tid2[HANDLETHREADS];			//Handling threads for SIGUSR2
pthread_t sid[SIGTHREADS];				//Signal Generating Threads
int block[NUMTHREADS];


sig_atomic_t sigSent1 = 0;
sig_atomic_t sigSent2 = 0;
sig_atomic_t sharedSignal = 0;
sig_atomic_t sigReceive1 = 0;
sig_atomic_t sigReceive2 = 0;

struct sigaction action;
//TODO: add error handling
int main(){

	int i;
	srand(time(NULL));
	
	//Set up/Install signal handlers for SIGUSR1 & SIGUSR2
	action.sa_flags = 0;
	action.sa_handler = signalUser1;
	sigaction(SIGUSR1, &action, (struct sigaction *)0);
	action.sa_handler = signalUser2;
	sigaction(SIGUSR2, &action, (struct sigaction *)0);
	int statSig, statH1, statH2;
	int status[2];
	
	//Signal generating threads
	for(i = 0; i < SIGTHREADS; i++){
		statSig = pthread_create(&sid[i], NULL, sig_handler, NULL);
		printf("signal generating thread %d created; Code_Stat: %d\n", i, statSig); 
	}
	
	//Handling threads for SIGUSR1
	for(i = 0; i < HANDLETHREADS; i++){
		statH1 = pthread_create(&tid1[i], NULL, handler1, NULL); 
		printf("handling thread SIGUSR1 %d created; Code_Stat: %d\n", i, statH1); 
	}
	
	//Handling threads for SIGUSR2
	for(i = 0; i < HANDLETHREADS; i++){
		statH2 = pthread_create(&tid2[i], NULL, handler2, NULL); 
		printf("handling thread SIGUSR2 %d created; Code_Stat: %d\n", i, statH2); 
	}
	
	int joinSig, joinH1, joinH2;
	//Wait on generating threads
	for(i = 0; i < SIGTHREADS; i++){
		 joinSig = pthread_join(sid[i], (void*) &status);
		 printf("Join Sig: %d\n", joinSig);
	}
	printf("--Waiting on SIGS complete\n");
	
	//Wait on threads for SIGUSR1
	for(i = 0; i < HANDLETHREADS; i++){
		pthread_join(tid1[i], (void*) &status);
		printf("Join H1: %d\n", joinH1);
	}
	printf("--Waiting on HANDLE1 complete\n");
	
	//Wait on threads for SIGUSR2
	for(i = 0; i < HANDLETHREADS; i++){
		pthread_join(tid2[i], (void*) &status);
		printf("Join H2: %d\n", joinH2);
	}
	printf("--Waiting on HANDLE2 complete\n");
	printf("\n\nSignal Sent 1 %d\n", sigSent1);
	printf("\n\nSignal Sent 2 %d\n", sigSent2);

	return 0;
}

void signalUser1(){
	printf("\n*** SIGUSR1 recieved by TID(%lu) ***\n", pthread_self());
}

void signalUser2(){
	printf("\n*** SIGUSR2 recieved by TID(%lu) ***\n", pthread_self());
}

float randNum(float start, float end){
	float diff = end - start;	//assume end is bigger than start
	float random = (float)rand()/(float)(RAND_MAX) * diff + start;
	return random;
}

int randBinary(){
	return rand() % 2;
}


void* sig_handler(){
	printf("\nSIGNAL GENERATOR CALLED - SID(%lu)\n", pthread_self());
	int signo, num;
	while(sharedSignal < SIGCOUNT){
		signo = randBinary();
		
		if(signo == 0){ 		//signo == SIGUSR1
			num = randBinary();
			printf("\tSIGNAL 1 is generated to array1[%d]\n", num);
			if(pthread_kill(tid1[num], SIGUSR1) != 0){
				perror("SIGUSR1 ERROR\n");
			}
			sigSent1++;
		} else {  //signo == SIGUSR2
			num = randBinary();
			printf("\tSignal 2 is generated to array2[%d]\n", num);
			if(pthread_kill(tid2[num], SIGUSR2) != 0){
				perror("SIGUSR2 ERROR\n");
			}
			sigSent2++;
		}
		sharedSignal++;
		sleep(randNum(0.1, 1));
	}
}

void* handler1(){
	printf("\n---HANDLER 1 CALLED---\n");
	siginfo_t info;
	int x;
	int rc;
	/*
	struct sigaction s;
	s.sa_flags = 0;
	s.sa_handler = SIG_IGN;
	*/
	sigemptyset(&action.sa_mask);
	sigaddset(&action.sa_mask, SIGUSR2);		//Block SIGUSR2
	//sigaction(SIGUSR1, &s, NULL);
	while(1){
		x = pthread_sigmask(SIG_BLOCK, &action.sa_mask, NULL);
		printf("%s\tReturned SIGUSR1 mask %d\n%s", BLUE, x, WHITE);
		sigReceive1++;
		printf("%s---SIGUSR1 Received Count: %d\n%s", BLUE, sigReceive1, WHITE);
		int sig;
		if((rc = sigwaitinfo(&action.sa_mask, &info)) <= 0){
			fprintf(stderr,"%sERROR SIGWAITINFO() 1!%s %d %s\n", RED, WHITE, rc, 
					strerror(errno));
		}else {
			fprintf(stdout,"SUCCESS!!!!!\n");
		}
	}	
}

void* handler2(){
	printf("\n---HANDLER 2 CALLED---\n");
	siginfo_t info;
	int x;
	int rc;
	/*
	struct sigaction s;
	s.sa_flags = 0;
	s.sa_handler = SIG_IGN;
	*/
	sigemptyset(&action.sa_mask);
	sigaddset(&action.sa_mask, SIGUSR1);		//Block SIGUSR1
	//sigaction(SIGUSR2, &s, NULL);

	while(1){
		x = pthread_sigmask(SIG_BLOCK, &action.sa_mask, NULL);
		printf("%s\tReturned SIGUSR2 mask %d\n%s", GREEN, x, WHITE);
		sigReceive2++;
		printf("%s---SIGUSR2 Received Count: %d\n%s", GREEN, sigReceive2, WHITE);
		int sig;
		if((rc = sigwaitinfo(&action.sa_mask, &info)) <= 0){
			fprintf(stderr,"%sERROR SIGWAITINFO() 2!%s %d %s\n", RED, WHITE, rc, 
					strerror(errno));
		} else {
			fprintf(stdout,"SUCCESS!!!!!\n");
		}
	}	
}









  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
