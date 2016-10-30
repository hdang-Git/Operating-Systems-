#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <semaphore.h>

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

volatile int sigSent1 = 0;
volatile int sigSent2 = 0;
volatile int sharedSignal = 0;
volatile int sigReceive1 = 0; 
volatile int sigReceive2 = 0;

pthread_cond_t got_request1 = PTHREAD_COND_INITIALIZER; 
pthread_cond_t got_request2 = PTHREAD_COND_INITIALIZER; 

pthread_mutex_t sigS1  = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sigS2  = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t shared = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sigR1  = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sigR2  = PTHREAD_MUTEX_INITIALIZER;

int main(){

	int i;
	srand(time(NULL));

	//Set up/Install signal handlers for SIGUSR1 & SIGUSR2
	sigset_t mask;
	/*
	sigemptyset(&mask);
	sigaddset(&mask, SIGUSR1);
	sigaddset(&mask, SIGUSR2);
	*/
	sigfillset(&mask);
	
	pthread_sigmask(SIG_BLOCK, &mask, NULL); //BLOCK IT ALL 

	int statSig, statH1, statH2;
	int status[2];
	
	
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

	//Signal generating threads
	for(i = 0; i < SIGTHREADS; i++){
		statSig = pthread_create(&sid[i], NULL, sig_handler, NULL);
		printf("signal generating thread %d created; Code_Stat: %d\n", i, statSig); 
	}
	sleep(3);

	int joinSig, joinH1, joinH2;

	//Wait on generating threads
	for(i = 0; i < SIGTHREADS; i++){
		 joinSig = pthread_join(sid[i], (void*) &status);
		 printf("Join Sig: %d\n", joinSig);
	}
	printf("--Waiting on SIGS complete\n");
	sleep(3);
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
	int signo, num, i;
	
	
	while(sharedSignal < SIGCOUNT){
		printf("shared signal: %d\n", sharedSignal);
		signo = randBinary();
		if(signo == 0){ 		//signo == SIGUSR1
			//for(i = 0; i < 2; i++){
			num = rand() % 2;
			printf("\tSIGNAL 1 is generated to array1[%d]\n", num);
			if(pthread_kill(tid1[num], SIGUSR1) != 0){ //num was i
				perror("SIGUSR1 ERROR\n");
			}
			
			//sem_wait(&sigS1);
			pthread_mutex_lock(&sigS1);
			sigSent1++;
			//sem_post(&sigS1);
			pthread_mutex_unlock(&sigS1);
			//}
		} else {  //signo == SIGUSR2
			//for(i = 0; i < 2; i++){
			num = randBinary();
			printf("\tSignal 2 is generated to array2[%d]\n", num);
			if(pthread_kill(tid2[num], SIGUSR2) != 0){
				perror("SIGUSR2 ERROR\n");
			}
			pthread_mutex_lock(&sigS2);
			//sem_wait(&sigS2);
			sigSent2++;
			//sem_post(&sigS2);
			pthread_mutex_unlock(&sigS2);
			//}

		}
		
		pthread_mutex_lock(&shared);
		sharedSignal++;
		pthread_mutex_unlock(&shared);
			
		//sleepMs(randNum(0.1, 1));
		sleep(1);
	}
}

void* handler1(){
	printf("\n---HANDLER 1 CALLED--- %lu\n", pthread_self());
	siginfo_t info;
	int x;
	int rc;
	int sig;
	struct timespec timeout;
	sigset_t mask, sigmask;
	
	//block all signals
	/*
	sigfillset(&mask); 
	x = pthread_sigmask(SIG_BLOCK, &mask, NULL);
	printf("%s\tReturned SIGUSR1 mask %d\n%s", BLUE, x, WHITE);
	*/
	//build different signal set to wait on sigusr1 
	sigemptyset(&sigmask);
	sigaddset(&sigmask, SIGUSR1);
	timeout.tv_sec = 3;
	timeout.tv_nsec = 0;
	while((rc = sigtimedwait(&sigmask, NULL, &timeout)) > 0){
		printf("SUCCESS! %lu\n", pthread_self());
		pthread_mutex_lock(&sigR1);	
		sigReceive1++;
		printf("%s---SIGUSR1 Received Count: %d\n%s", BLUE, sigReceive1, WHITE);
		//int c = pthread_cond_signal(&got_request1); 
		pthread_mutex_unlock(&sigR1);

	}	
	if(rc  <= 0){
		fprintf(stderr,"%sERROR SIGWAIT() 1!%s %d %s tid[%lu]\n", RED, WHITE, rc, 
				strerror(errno), pthread_self());
	}
	pthread_sigmask(SIG_UNBLOCK, &mask, NULL);
}

void* handler2(){
	printf("\n---HANDLER 2 CALLED---%lu\n", pthread_self());
	siginfo_t info;
	int x;
	int rc;
	struct timespec timeout;
	sigset_t mask, sigmask;
	/*
	//block all signals
	sigfillset(&mask); 
	x = pthread_sigmask(SIG_BLOCK, &mask, NULL);
	printf("%s\tReturned SIGUSR2 mask %d\n%s", GREEN, x, WHITE);
	*/
	//build different signal set to wait on sigusr2 
	sigemptyset(&sigmask);
	sigaddset(&sigmask, SIGUSR2);

	
	timeout.tv_sec = 3;
	timeout.tv_nsec = 0;
	while((rc = sigtimedwait(&sigmask, NULL, &timeout)) > 0){
		printf("SUCCESS! %lu\n", pthread_self());
		pthread_mutex_lock(&sigR2);
		sigReceive2++;
		printf("%s---SIGUSR2 Received Count: %d\n%s", GREEN, sigReceive2, WHITE);
		//int c = pthread_cond_signal(&got_request2); 
		pthread_mutex_unlock(&sigR2);
	}	
	
	if(rc <= 0){
			fprintf(stderr,"%sERROR SIGWAIT() 2!%s %d %s tid[%lu]\n", RED, WHITE, rc, 
					strerror(errno), pthread_self());
	} 
	pthread_sigmask(SIG_UNBLOCK, &mask, NULL);
}









  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
