// Michal Bochnak
// Netid: mbochn2
// CS 361
// Oct 5, 2017
//
// shell.cpp
//

/*
Notes:
	- when do I need to block signal / use write
	- Ctrl + Z should put printing ones in background?
    - ended child, print "exited" / "uncought signal"
    - how to use kill command
	- termination status aslways 0? number of signals always 0?
*/


#include <iostream>
#include <cstring>
#include <algorithm>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <cstdio> 		// perror
#include <setjmp.h>

using namespace std;

// function prototypes
std::string getInput();
char **generateTokens();
void displayTokens(char **t);
void showInfoAboutChildSignals(int a, int status);
void sigusr1_handler(int sig); 
void sigint_handler(int sig);
void sigtstp_handler(int sig) ;

// globals
struct rusage rusage_struct;
int precisionCount = 0;
int child_pid;
int precision = 3;
sigjmp_buf envBuffer;
bool haveChild = false;	// for SIGINT SITSTP handlers


int main(int argc, char* argv[]) {

	signal(SIGUSR1, sigusr1_handler);
	signal(SIGINT, sigint_handler);	
	signal(SIGTSTP, sigtstp_handler);

	// blocl signals
	sigset_t mask, old;
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigprocmask(SIG_BLOCK, &mask, &old);
	
	// info
	printf("Program: Simple Shell\nAuthor: Michal Bochnak\nNetid: mbochn2\n\n");

	// check if precision argument was given
	if ((argc > 1) && (atoi(argv[1]) > 1)) {
		precision = atoi(argv[1]);
	}
	sigprocmask(SIG_SETMASK, &old, NULL);

	// get the user input line and convert it to the array
	// of null terminated pointers to tokens from the line
	char **tokens = generateTokens();
	

	// process the user command until he enters exit
	while ((tokens[0] != NULL) && (strcmp(tokens[0], "exit") != 0)) {
		// block signals
        sigemptyset(&mask);
        sigaddset(&mask, SIGINT);
        sigprocmask(SIG_BLOCK, &mask, &old);	
		
		precisionCount = 0;
		child_pid = fork();
		haveChild = true;

		sigprocmask(SIG_SETMASK, &old, NULL);

		int wchild_status;

		// error, child pid less than 0
		if (child_pid < 0) {
			perror("fork");
			exit(EXIT_FAILURE);
		}
		// this is executed by child
		if (child_pid == 0) {
			if (execvp(tokens[0], tokens) < 0) {
				write(1, "execv failed\n", 13);
				exit(0);
			}
		}
		// executed by parent
		else {
			// show child PID
			printf("Forked Child pid: %d\n", child_pid);

			// enviroinment point
			sigsetjmp(envBuffer, 1);
			
			// wait for the child
			pid_t wpid = wait4(child_pid, &wchild_status, 0, &rusage_struct);

			haveChild = false;
			showInfoAboutChildSignals(wpid, wchild_status);
			tokens = generateTokens();
		}

	}

	return 0;
}


// SIGINT
void sigint_handler(int sig) {
	if (haveChild == true)
		siglongjmp(envBuffer, 1);
	else 
		exit(0);
}

// SIGTSTP
void sigtstp_handler(int sig) {
    if (haveChild == true)
		siglongjmp(envBuffer, 1);
	else 
		exit(0);   
}

// SIGUSR1
void sigusr1_handler(int sig) {
	precisionCount++;
	if (precisionCount >= precision) {
		// send SIGUSR2 by kill to child
		kill(child_pid, SIGUSR2);
	}
}


// get the input line from user and return it 
std::string getInput() {

	std::string line;
	// prompt for input
	std::cout << "Type the command in: ";
	std::getline(std::cin, line);
	
	return line;
}

// generates the array of pointers to the tokens
// from the given line of input
char **generateTokens() {

	// get the line inputted by user
	std::string line = getInput();

	// count the tokens in the string to use to
	// initialize the size of the array pointers
	// starting from value 1, since the end of the string
	// might not contain a space
	int numTokens = 1;
	for (int i = 0; i < line.length(); ++i) {
		if (line[i] == ' ') {
			numTokens++;
		}
	}

	// malloc the memory for the pointers
	char **tokens = (char**)malloc(sizeof(char*) * numTokens);

	// convert 'line' string to C-style null-terminated char pointer
	char *command = new char[line.length() + 1];
	std::strcpy(command, line.c_str());

	// parse the line
	int i = 0;
	tokens[i] = strtok(command, " ");
	while (tokens[i] != NULL) {
		//std::cout << tokens[i] << std::endl;
		++i;
		tokens[i] = strtok(NULL, " ");
	}

	// return the array of poiters to the parsed tokens
	return tokens;
}

// shows info about the child signals
void showInfoAboutChildSignals(int a, int status) {
	// block signal
	sigset_t mask, old;
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigprocmask(SIG_BLOCK, &mask, &old);	

	printf("Child %d teminated with status %d\n", 
			a, WEXITSTATUS(status) );
	printf("Child number of page faults: %d\n", 
			rusage_struct.ru_minflt + rusage_struct.ru_majflt);
	printf("Child number of signals received: %d\n", 
			rusage_struct.ru_nsignals);

	sigprocmask(SIG_SETMASK, &old, NULL);
}


