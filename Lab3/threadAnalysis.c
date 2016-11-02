#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include<sys/time.h>
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
void* reporter();
float randNum(float, float);
int randBinary();
void signalUser1();
void signalUser2();


pthread_t tid1[HANDLETHREADS];			//Handling threads for SIGUSR1
pthread_t tid2[HANDLETHREADS];			//Handling threads for SIGUSR2
pthread_t sid[SIGTHREADS];				//Signal Generating Threads
pthread_t rid;							//Reporter Thread
int block[NUMTHREADS];
struct timespec timeout;

volatile int sigSent1 = 0;
volatile int sigSent2 = 0;
volatile int sharedSignal = 0;
volatile int sigReceive1 = 0; 
volatile int sigReceive2 = 0;

pthread_mutex_t sigS1  = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sigS2  = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t shared = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sigR1  = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sigR2  = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t reportR1  = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t reportR2  = PTHREAD_MUTEX_INITIALIZER;

struct timeval t1;
struct timeval t2;
static long int start1;
static long int start2;
static long int end1;
static long int end2;

int main(){

	int i;
	srand(time(NULL));
	int statSig, statH1, statH2, statR;
	int status[2];

	//Either block SIGUSR1 & SIGUSR2 from main thread by blocking everything or install 
	//signal handlers for it.
	//Signal handlers will be overridden in individual threads
	/*
	struct sigaction action;
	action.sa_flags = 0;
	action.sa_handler = signalUser1;
	sigaction(SIGUSR1, &action, NULL);
	action.sa_handler = signalUser2;
	sigaction(SIGUSR2, &action, NULL);
	*/
	/*
	signal(SIGUSR1, signalUser1);
	signal(SIGUSR2, signalUser2);
	*/
	
	/*
	//Set up/Install signal handlers for SIGUSR1 & SIGUSR2
	sigset_t mask;
	sigfillset(&mask);
	pthread_sigmask(SIG_BLOCK, &mask, NULL); //BLOCK IT ALL 
	*/
	sigset_t mask;
	sigaddset(&mask, SIGUSR1);
	sigaddset(&mask, SIGUSR2);
	pthread_sigmask(SIG_BLOCK, &mask, NULL); //BLOCK SIGUSR1, SIGUSR2 in all threads 

	//Reporting thread
	statR = pthread_create(&rid, NULL, reporter, NULL);
	printf("reporting thread created; Code_Stat: %d\n", statR);
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

	//sleep(3);
	
	//Signal generating threads
	for(i = 0; i < SIGTHREADS; i++){
		statSig = pthread_create(&sid[i], NULL, sig_handler, NULL);
		printf("signal generating thread %d created; Code_Stat: %d\n", i, statSig); 
	}
	sleep(3);

	int joinSig, joinH1, joinH2, joinR;

	//Wait on generating threads
	for(i = 0; i < SIGTHREADS; i++){
		 joinSig = pthread_join(sid[i], (void*) &status);
		 printf("Join Sig: %d\n", joinSig);
	}
	printf("--Waiting on SIGS complete\n");
	sleep(3);
	//Wait on threads for SIGUSR1
	for(i = 0; i < HANDLETHREADS; i++){
		joinH1 = pthread_join(tid1[i], (void*) &status);
		printf("Join H1: %d\n", joinH1);
	}
	printf("--Waiting on HANDLE1 complete\n");
	
	//Wait on threads for SIGUSR2
	for(i = 0; i < HANDLETHREADS; i++){
		joinH2 = pthread_join(tid2[i], (void*) &status);
		printf("Join H2: %d\n", joinH2);
	}
	//Wait on Reporter thread
	joinR = pthread_join(rid, (void*) &status);
	printf("Join Reporter %d\n", joinR);
	
	printf("--Waiting on HANDLE2 complete\n\n");
	printf("Signal Sent 1 %d\n", sigSent1);
	printf("Signal Sent 2 %d\n", sigSent2);
	printf("Signal Received 1 %d\n", sigReceive1);
	printf("Signal Received 2 %d\n", sigReceive2);
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

void signalUser1(){
	printf("\n*** SIGUSR1 recieved by TID(%lu) ***\n", pthread_self());
}

void signalUser2(){
	printf("\n*** SIGUSR2 recieved by TID(%lu) ***\n", pthread_self());
}

void* sig_handler(){
	printf("\nSIGNAL GENERATOR CALLED - SID(%lu)\n", pthread_self());
	int signo, num, i;
	
	
	while(sharedSignal < SIGCOUNT){
		printf("shared signal: %d\n", sharedSignal);
		signo = randBinary();
		if(signo == 0){ 		//signo == SIGUSR1
			num = rand() % 2;
			printf("\tSIGNAL 1 is generated to array1[%d]\n", num);
			
			if(pthread_kill(tid1[num], SIGUSR1) != 0){ 
				perror("SIGUSR1 ERROR\n");
			}
			
			printf("\tSIGNAL 1 is generated to rid\n");
			if(i = pthread_kill(rid, SIGUSR1) < 0){ 
				perror("SIGUSR1 Report Generator ERROR\n");
				fprintf(stderr,"%sERROR REPORTER SIGNAL!%s %d %s tid[%lu]\n", RED, WHITE, i, 
					strerror(errno), pthread_self());
			}
			//sem_wait(&sigS1);
			pthread_mutex_lock(&sigS1);
			sigSent1++;
			//sem_post(&sigS1);
			pthread_mutex_unlock(&sigS1);

		} else {  //signo == SIGUSR2
			num = randBinary();
			printf("\tSignal 2 is generated to array2[%d]\n", num);
			
			if(pthread_kill(tid2[num], SIGUSR2) != 0){
				perror("SIGUSR2 ERROR\n");
			}
			
			printf("\tSIGNAL 2 is generated to rid\n");
			if(i = pthread_kill(rid, SIGUSR2) < 0){ 
				perror("SIGUSR2 Reporter Generator ERROR\n");
				fprintf(stderr,"%sERROR REPORTER SIGNAL!%s %d %s tid[%lu]\n", RED, WHITE, i, 
					strerror(errno), pthread_self());
			}
			
			pthread_mutex_lock(&sigS2);
			//sem_wait(&sigS2);
			sigSent2++;
			//sem_post(&sigS2);
			pthread_mutex_unlock(&sigS2);

		}
		
		pthread_mutex_lock(&shared);
		sharedSignal++;
		pthread_mutex_unlock(&shared);
			
		//sleepMs(randNum(0.1, 1));
		sleep(randNum(0.1, 1));
	}
}

//TODO: reexamine pthread_sigmask - might not be doing anything
void* handler1(){
	printf("\n---HANDLER 1 CALLED--- %lu\n", pthread_self());
	siginfo_t info;
	int x;
	int rc;
	int sig;
	sigset_t mask, sigmask;
	timeout.tv_sec = 3;
	timeout.tv_nsec = 0;
	//block all signals
	/*
	sigfillset(&mask); 
	x = pthread_sigmask(SIG_BLOCK, &mask, NULL);
	printf("%s\tReturned SIGUSR1 mask %d\n%s", BLUE, x, WHITE);
	*/
	//build different signal set to wait on sigusr1 

	sigemptyset(&sigmask);
	sigaddset(&sigmask, SIGUSR1);
	while((rc = sigtimedwait(&sigmask, NULL, &timeout)) > 0){
		printf("SUCCESS! %lu\n", pthread_self());
		pthread_mutex_lock(&sigR1);	
		sigReceive1++;
		printf("%s---SIGUSR1 Received Count: %d\n%s", BLUE, sigReceive1, WHITE);
		//int c = pthread_cond_signal(&got_request1); 
		pthread_mutex_unlock(&sigR1);

	}	
	if(sharedSignal < SIGCOUNT && rc  <= 0){
		fprintf(stderr,"%sERROR SIGWAIT() 1!%s %d %s tid[%lu]\n", RED, WHITE, rc, 
				strerror(errno), pthread_self());
	}

}

//TODO: reexamine pthread_sigmask - might not be doing anything
void* handler2(){
	printf("\n---HANDLER 2 CALLED---%lu\n", pthread_self());
	siginfo_t info;
	int x;
	int rc;
	timeout.tv_sec = 3;
	timeout.tv_nsec = 0;
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
	
	while((rc = sigtimedwait(&sigmask, NULL, &timeout)) > 0){
		printf("SUCCESS! %lu\n", pthread_self());
		pthread_mutex_lock(&sigR2);
		sigReceive2++;
		printf("%s---SIGUSR2 Received Count: %d\n%s", GREEN, sigReceive2, WHITE);
		//int c = pthread_cond_signal(&got_request2); 
		pthread_mutex_unlock(&sigR2);
	}	
	
	if(sharedSignal < SIGCOUNT && rc <= 0){
		fprintf(stderr,"%sERROR SIGWAIT() 2!%s %d %s tid[%lu]\n", RED, WHITE, rc, 
			strerror(errno), pthread_self());
	} 

}

void* reporter(){
	printf("\n!!!!!!!REPORTER!!!!!!!!!!!!\n");
	sigset_t mask, sigmask;
	sigfillset(&mask);
	sigemptyset(&sigmask);
	int sig;
	sigaddset(&sigmask, SIGUSR1);
	sigaddset(&sigmask, SIGUSR2);
	timeout.tv_sec = 3;
	timeout.tv_sec = 0;
	

	gettimeofday(&t1, NULL);			    //get the end time in microseconds
	gettimeofday(&t2, NULL);			    //get the end time in microseconds
	int rc;
	long int difference1, difference2, sum1 = 0, sum2 = 0;
	start1 = t1.tv_sec * 1000000L + t1.tv_usec;  //Convert s & us to us
	start2 = t2.tv_sec * 1000000L + t2.tv_usec;  //Convert s & us to us
	while(sharedSignal < SIGCOUNT){
		rc = sigwait(&sigmask, &sig);
		printf("In reporter\n");
		if(sig == SIGUSR1){
		//Lock 1
			pthread_mutex_lock(&sigR1);
			gettimeofday(&t1, NULL);			    //get the end time in microseconds
			
			printf("start1 time: %ld\n", start1);
			//Overwrite old time with new time
	   		end1 = t1.tv_sec * 1000000L + t1.tv_usec;  //Convert s & us to us
	   		printf("end1 time: %ld\n", end1);
	   		//calculate difference
	   		difference1 = end1 - start1; 
			printf("difference1 time: %ld\n", difference1);
			//accumulate with global variable
			sum1 += difference1;
			
			//store old time into start time
			start1 = end1;
			pthread_mutex_unlock(&sigR1);
		} else if (sig == SIGUSR2){
			//Lock 2
			pthread_mutex_lock(&sigR2);
			gettimeofday(&t2, NULL);			    //get the end time in microseconds
			
			printf("start2 time: %ld\n", start2);
			//Overwrite old time with new time
			end2 = t2.tv_sec * 1000000L + t2.tv_usec;  //Convert s & us to us
			printf("end2 time: %ld\n", end2);
	   		//calculate difference
	   		difference2 = end2 - start2; 
			printf("difference2 time: %ld\n", difference2);
			//accumulate with global variable
			sum2 += difference2;
			
			//store old time into start time
			start2 = end2;
			pthread_mutex_unlock(&sigR2);
		} 
		else if(rc <= 0){
			fprintf(stderr,"%sERROR SIGWAIT() REPORTER!%s %d %s tid[%lu]\n", RED, WHITE, rc, 
					strerror(errno), pthread_self());
		} 
		//For every ten signals, report the message
		if((sharedSignal+1)%10 == 0){
			char* msg = "Hello\n";
			write(STDOUT_FILENO, msg, strlen(msg));
		}
		
		if(sharedSignal == SIGCOUNT-1)
			break;
		
	}
	

	
}








  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
