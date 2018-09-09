// Michal Bochnak
// Netid: mbochn2
// CS 361
// Oct 5, 2017
//
// montecarlo.cpp
//

/*

*/

using namespace std;

// libraries
#include <time.h>
#include <iostream>
#include <algorithm>
#include <unistd.h>
#include <cstdio> 		// perror
#include <setjmp.h>
#include <limits.h>
#include <signal.h>


// globals
unsigned long int numOfEvalAttempted = 0;
unsigned long int segFaultsGenerated = 0;
double segFaultRatio = 0;
sigjmp_buf envBuf;
int displayCounter = 0;


// function declarations
void sigsegv_handler(int sig);
void sigint_handler(int sig);
void sigtstp_handler(int sig);
void sigalrm_handler(int sig);
void sigusr2_handler(int sig);
void reportResults();


int main (int argc, char* argv[]) {

    signal(SIGSEGV, sigsegv_handler);
    signal(SIGINT, sigint_handler);    
    signal(SIGUSR2, sigint_handler);    
    signal(SIGTSTP, sigtstp_handler);
    signal(SIGALRM, sigalrm_handler);

    srand(time(NULL));      // random
    int* randomNumIntPtr = NULL;
    int randomNumInt = 0;
    unsigned long int report = 10;

    // block signals
    sigset_t mask, old;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigprocmask(SIG_BLOCK, &mask, &old);	

    // program info
    printf("Program: Monte Carlo Simulation\nAuthor: Michal Bochnak\nNetid: mbochn2\n\n");

    sigprocmask(SIG_SETMASK, &old, NULL);
     
    if (argc > 1) {
        // set the alarm
        alarm(atoi(argv[1]));
    } 

    // generate random addresses
    for (; numOfEvalAttempted < ULONG_MAX; ++numOfEvalAttempted) {
        sigsetjmp(envBuf, 1);
        
        // sent signal to parent
        if (numOfEvalAttempted == report) {
            // send SIGUSR1 by kill command
            kill(getppid(), SIGUSR1);
            report *= 10;
        }        
       
       // generate random number
        randomNumIntPtr = (int*)rand();
        randomNumInt = *randomNumIntPtr;
    }

    reportResults();
 
    return 0;
}


// SIGSEGV
void sigsegv_handler(int sig) {

    // block signals
    sigset_t mask, old;
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigprocmask(SIG_BLOCK, &mask, &old);

    segFaultsGenerated++;
    numOfEvalAttempted++;
    
    sigprocmask(SIG_SETMASK, &old, NULL);

    // max reached
    if (numOfEvalAttempted == ULONG_MAX) { 
        reportResults();
        exit(0);
    }
    // max not reached
    else {
        siglongjmp(envBuf, 1);
    }
}

// SIGINT
void sigint_handler(int sig) {
    reportResults();
    exit(0);
}

// SIGTSTP
void sigtstp_handler(int sig) {
    reportResults();    
}

// SIGALRM
void sigalrm_handler(int sig) {
    reportResults();
    exit(0);
}

// report results
void reportResults() {

    // block signals
    sigset_t mask, old;
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigprocmask(SIG_BLOCK, &mask, &old);	

    printf("\nTotal segmentation faults:   %lu\n", segFaultsGenerated);
    printf("Total evaluations attempted: %lu\n", numOfEvalAttempted);
    printf("Percentage of segmentaion faults: %f%\n", 
            ((double)segFaultsGenerated / numOfEvalAttempted) * 100);

    sigprocmask(SIG_SETMASK, &old, NULL);
}

