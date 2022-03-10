#include "threadtools.h"

/*
1) You should state the signal you received by: printf('TSTP signal caught!\n') or printf('ALRM signal caught!\n')
2) If you receive SIGALRM, you should reset alarm() by timeslice argument passed in ./main
3) You should longjmp(SCHEDULER,1) once you're done.
*/
void sighandler(int signo){
	/* Please fill this code section. */
	if(signo==SIGTSTP){
		sigprocmask(SIG_SETMASK,&base_mask,NULL);
		printf("TSTP signal caught!\n");
		longjmp(SCHEDULER, 1);
	}
	else if(signo==SIGALRM){
		sigprocmask(SIG_SETMASK,&base_mask,NULL);
		alarm(timeslice);
		printf("ALRM signal caught!\n");
		longjmp(SCHEDULER,1);
	}
}

/*
1) You are stronly adviced to make 
	setjmp(SCHEDULER) = 1 for ThreadYield() case
	setjmp(SCHEDULER) = 2 for ThreadExit() case
2) Please point the Current TCB_ptr to correct TCB_NODE
3) Please maintain the circular linked-list here
*/
void scheduler(){
	/* Please fill this code section. */
	int type = setjmp(SCHEDULER);
	if(type==0||type==1){
		Work = Work->Next;
		longjmp(Work->Environment,1);
	}
	else{
		if(Work->Next==Work){
			free(Work);
			longjmp(MAIN,1);
		}
		else{
			TCB_ptr tmp = Work;
			Work->Prev->Next = Work->Next;
			Work->Next->Prev = Work->Prev;
			Work = Work->Next;
			free(tmp);
			longjmp(Work->Environment,1);
		}
	}
}
