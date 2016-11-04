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
#define SIGCOUNT 30
#define SIGNUM 10
#define WHITE "\x1B[0m"
#define GREEN "\x1B[32m"
#define BLUE "\x1B[34m"
#define MAGENTA "\x1B[35m"
#define RED "\e[41m"

//function prototypes
void* sig_handler();
void* handler1();
void* handler2();
void* reporter();
float randNum(float, float);
int randBinary();

pthread_t tid1[HANDLETHREADS];			//Handling threads for SIGUSR1
pthread_t tid2[HANDLETHREADS];			//Handling threads for SIGUSR2
pthread_t sid[SIGTHREADS];				//Signal Generating Threads
pthread_t rid;							//Reporter Thread
struct timespec timeout;

volatile int sigSent1 = 0;				//Counter for total sent SIGUSR1 signals
volatile int sigSent2 = 0;				//Counter for total sent SIGUSR2 signals
volatile int sharedSignal = 0;			//Counter for total signals sent
volatile int sigReceive1 = 0; 			//Counter for total received SIGUSR1 signals
volatile int sigReceive2 = 0;			//Counter for total received SIGUSR2 signals
volatile int flag = 0;

volatile int loc_sigReceive1 = 0; 			//Counter for received SIGUSR1 signals
volatile int loc_sigReceive2 = 0;			//Counter for received SIGUSR2 signals
volatile int total = 0;
volatile int count1 = 0;
volatile int count2 = 0;
//Mutexes
pthread_mutex_t sigS1  = PTHREAD_MUTEX_INITIALIZER;		
pthread_mutex_t sigS2  = PTHREAD_MUTEX_INITIALIZER;		
pthread_mutex_t shared = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sigR1  = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sigR2  = PTHREAD_MUTEX_INITIALIZER;

struct timeval t1;
struct timeval t2;
static long int start1;				//Start time for SIGUSR1 received
static long int start2;				//Start time for SIGUSR2 received
static long int end1;				//End time for SIGUSR1 received
static long int end2;				//Ene time for SIGUSR2 received

int main(){
	int i;
	srand(time(NULL));
	int status[2];
	
	//BLOCK SIGUSR1, SIGUSR2 in all threads;
	//Mask will be inherited by child threads and overridden.
	sigset_t mask;
	sigaddset(&mask, SIGUSR1);
	sigaddset(&mask, SIGUSR2);
	pthread_sigmask(SIG_BLOCK, &mask, NULL); 

	//Create Reporting thread
	if(pthread_create(&rid, NULL, reporter, NULL) != 0){
		perror("Reporting Thread Creation Error.\n");
	}
	//Create Handler threads for SIGUSR1
	for(i = 0; i < HANDLETHREADS; i++){
		if(pthread_create(&tid1[i], NULL, handler1, NULL) != 0){
			fprintf(stderr, "Handler 1 Thread %d Creation Error; %s\n", 
			i, strerror(errno));
		}
	}
	//Create Handler threads for SIGUSR2
	for(i = 0; i < HANDLETHREADS; i++){
		if(pthread_create(&tid2[i], NULL, handler2, NULL) != 0){
			fprintf(stderr, "Handler 2 Thread %d Creation Error; %s\n", 
			i, strerror(errno));
		}
	}
	//sleep(3);
	//Create Signal generating threads
	for(i = 0; i < SIGTHREADS; i++){
		if(pthread_create(&sid[i], NULL, sig_handler, NULL) != 0){
			fprintf(stderr, "Signal Generating Thread %d Creation Error; %s\n", 
			i, strerror(errno));
		}
	}
	sleep(3);
	//Wait on generating threads
	for(i = 0; i < SIGTHREADS; i++){
		 if(pthread_join(sid[i], (void*) &status) != 0){
		 	fprintf(stderr, "Signal Generating Thread %d Join Error; %s\n", 
			i, strerror(errno));
		 }
	}
	printf("--Waiting on SIGS complete\n");
	sleep(3);
	//Wait on threads for SIGUSR1
	for(i = 0; i < HANDLETHREADS; i++){
		if(pthread_join(tid1[i], (void*) &status) != 0){
			fprintf(stderr, "Handler 1 Thread %d Join Error; %s\n", 
			i, strerror(errno));
		}
	}
	printf("--Waiting on HANDLE1 complete\n");
	//Wait on threads for SIGUSR2
	for(i = 0; i < HANDLETHREADS; i++){
		if(pthread_join(tid2[i], (void*) &status) != 0){
			fprintf(stderr, "Handler 2 Thread %d Join Error; %s\n", 
			i, strerror(errno));
		}
	}
	//Wait on Reporter thread
	if(pthread_join(rid, (void*) &status) != 0){
		perror("Error Joining Reporter Thread.\n");
	}

	
	printf("--Waiting on HANDLE2 complete\n\n");
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
	int signo, num;
	//Loop as long as the signal count is less than the specified SIGCOUNT
	while(sharedSignal < SIGCOUNT){
		pthread_mutex_lock(&shared);
		//Randomly choose a random number to choose a signal
		signo = randBinary();				
		if(signo == 0){ 		//signo == SIGUSR1
			num = rand() % 2;
			//Send signal to random handler thread for SIGUSR1
			if(pthread_kill(tid1[num], SIGUSR1) != 0){ 
				perror("SIGUSR1 ERROR\n");
			}
			//Send SIGUSR1 signal to reporter thread
			if(pthread_kill(rid, SIGUSR1) < 0){ 
				perror("SIGUSR1 Report Generator ERROR\n");
			}
			//Increment shared SIGUSR1 signal sent counter
			pthread_mutex_lock(&sigS1);
			sigSent1++;
			pthread_mutex_unlock(&sigS1);

		} else {  //signo == SIGUSR2
			//Randomly choose a random number to choose a signal
			num = randBinary();	
			//Send signal to random handler thread for SIGUSR2
			if(pthread_kill(tid2[num], SIGUSR2) != 0){
				perror("SIGUSR2 ERROR\n");
			}
			//Send SIGUSR2 signal to reporter thread
			if(pthread_kill(rid, SIGUSR2) < 0){ 
				perror("SIGUSR2 Reporter Generator ERROR\n");
			}
			//Increment shared SIGUSR2 signal sent counter
			pthread_mutex_lock(&sigS2);
			sigSent2++;
			pthread_mutex_unlock(&sigS2);
		}
		//Increment shared total signal counter
		sharedSignal++;
		//If shared total signal counter is a multiple of ten, set flag to 1
		if(((sharedSignal + 1) %10) == 0){
			flag = 1;
		}
		pthread_mutex_unlock(&shared);
		//Sleep for a random interval of time 
		sleep(randNum(0.01, 0.1));
	}
}


void* handler1(){
	printf("\n---HANDLER 1 CALLED--- %lu\n", pthread_self());
	siginfo_t info;
	int x;
	int rc;
	int sig;
	sigset_t sigmask;
	timeout.tv_sec = 0;
	timeout.tv_nsec = 3;

	//build different signal set to wait on sigusr1 
	sigemptyset(&sigmask);
	sigaddset(&sigmask, SIGUSR1);
	while((rc = sigtimedwait(&sigmask, NULL, &timeout)) > 0){
		printf("SUCCESS! %lu\n", pthread_self());
		pthread_mutex_lock(&sigR1);	
		sigReceive1++;
		loc_sigReceive1++;
		if(sigReceive1 > SIGNUM)
			loc_sigReceive1 = 0;
		printf("%s---SIGUSR1 Received Count: %d\n%s", BLUE, sigReceive1, WHITE);
		pthread_mutex_unlock(&sigR1);

	}	
	if(sharedSignal < SIGCOUNT && rc  <= 0){
		fprintf(stderr,"%sERROR SIGWAIT() 1!%s %d %s tid[%lu]\n", RED, WHITE, rc, 
				strerror(errno), pthread_self());
	}

}


void* handler2(){
	int rc;
	timeout.tv_sec = 0;
	timeout.tv_nsec = 3;
	sigset_t sigmask;

	//build different signal set to wait on sigusr2 
	sigemptyset(&sigmask);
	sigaddset(&sigmask, SIGUSR2);
	//
	while((rc = sigtimedwait(&sigmask, NULL, &timeout)) > 0){
		pthread_mutex_lock(&sigR2);
		sigReceive2++;
		loc_sigReceive2++;
		if(sigReceive1 > SIGNUM)
			loc_sigReceive2 = 0;
		printf("%s---SIGUSR2 Received Count: %d\n%s", GREEN, sigReceive2, WHITE);
		pthread_mutex_unlock(&sigR2);
	}	
	
	if(sharedSignal < SIGCOUNT && rc <= 0){
		fprintf(stderr,"%sERROR SIGWAIT() 2!%s %d %s tid[%lu]\n", RED, WHITE, rc, 
			strerror(errno), pthread_self());
	} 

}

void* reporter(){
	int sig, rc;
	long int difference1, difference2, sum1 = 0, sum2 = 0;
	float average1 = 0.0, average2 = 0.0;
	//Get current datetime
	time_t dateTime;
	dateTime = time(NULL);
	
	//Create signal set to wait on both SIGUSR1 and SIGUSR2 
	sigset_t sigmask;
	sigemptyset(&sigmask);
	sigaddset(&sigmask, SIGUSR1);
	sigaddset(&sigmask, SIGUSR2);
	
	gettimeofday(&t1, NULL);			    //get the end time in microseconds
	gettimeofday(&t2, NULL);			    //get the end time in microseconds
	start1 = t1.tv_sec * 1000000L + t1.tv_usec;  //Convert s & us to us
	start2 = t2.tv_sec * 1000000L + t2.tv_usec;  //Convert s & us to us
	//Loop while the signal count is less than the specified total count 
	while(sharedSignal < SIGCOUNT){
		rc = sigwait(&sigmask, &sig);
		if(sig == SIGUSR1){
			//Lock 1
			pthread_mutex_lock(&sigR1);
			//get the end time in microseconds
			gettimeofday(&t1, NULL);			   
			//Overwrite old time with new time
	   		end1 = t1.tv_sec * 1000000L + t1.tv_usec;  //Convert s & us to us
	   		//calculate difference
	   		difference1 = end1 - start1; 
			//accumulate with time differences of SIGUSR1
			sum1 += difference1;
			//store old time into start time
			start1 = end1;
			//Increment counter for SIGUSR1
			count1++;
			total++;
			pthread_mutex_unlock(&sigR1);
		} else if (sig == SIGUSR2){
			//Lock 2
			pthread_mutex_lock(&sigR2);
			//get the end time in microseconds
			gettimeofday(&t2, NULL);			    
			//Overwrite old time with new time
			end2 = t2.tv_sec * 1000000L + t2.tv_usec;  //Convert s & us to us
	   		//calculate difference
	   		difference2 = end2 - start2; 
			//accumulate with time differences of SIGUSR2
			sum2 += difference2;
			//store old time into start time
			start2 = end2;
			count2++;
			total++;
			pthread_mutex_unlock(&sigR2);
		} 
		else if(rc <= 0){
			fprintf(stderr,"%sERROR SIGWAIT() REPORTER!%s %d %s tid[%lu]\n", 
					RED, WHITE, rc, strerror(errno), pthread_self());
		} 

		//For every ten signals, report the message
		if(flag){
			printf("%s\n", asctime(localtime(&dateTime)));
			//Compute averages for each signal type in reporter.
			average1 = sum1/(float)count1;
			average2 = sum2/(float)count2;
			printf("SharedSig: %d, total: %d, SUM1: %ld, SUM2: %ld, Count1: %d, Count2: %d\n", 
					sharedSignal+1, total + 1, sum1, sum2, count1, count2);
			printf("New count: 1: %d 2: %d\n", loc_sigReceive1, loc_sigReceive2);
			printf("Average for SIGUSR1: %f\n", average1);
			printf("Average for SIGUSR2: %f\n", average2);
			printf("Average 1 : %f\n", sum1/(float)(sigReceive1%10));
			printf("Average 2 : %f\n", sum2/(float)(sigReceive2%10));
			printf("Average1 new: %f\n", sum1/(float)loc_sigReceive1);
			printf("Average2 new: %f\n", sum1/(float)loc_sigReceive2);
			printf("Signal Sent 1: %d\n", sigSent1);
			printf("Signal Sent 2: %d\n", sigSent2);
			printf("Signal Received 1: %d\n", sigReceive1);
			printf("Signal Received 2: %d\n\n", sigReceive2);
			
			//Reset counters and flag
			count1 = 0;
			count2 = 0;
			total = 0;
			flag = 0;
		}
		//TODO: delete
		if(sharedSignal == SIGCOUNT-1)
			break;
		
	}
}








  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
