#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>

extern int timeslice, switchmode;

typedef struct TCB_NODE *TCB_ptr;
typedef struct TCB_NODE{
    jmp_buf  Environment;
    int      Thread_id;
    TCB_ptr  Next;
    TCB_ptr  Prev;
    int i, N;
    int w, x, y, z;
} TCB;

extern jmp_buf MAIN,SCHEDULER;
extern TCB_ptr Head;
extern TCB_ptr Current;
extern TCB_ptr Work;
extern sigset_t base_mask, waiting_mask, tstp_mask, alrm_mask;

void sighandler(int signo);
void scheduler();

// Call function in the argument that is passed in
#define ThreadCreate(function, thread_id, number) \
{\
    TCB_ptr node = (TCB_ptr)malloc(sizeof(TCB));\
    Work = node;\
    function(thread_id,number);\
}                                        

// Build up TCB_NODE for each function, insert it into circular linked-list
#define ThreadInit(thread_id, number) \
{\
    Work->Thread_id = thread_id;\
    Work->N = number;\
    Work->x=1,Work->y=1,Work->z=0,Work->i=2;\
    if(Work->Thread_id==1){\
        Head = Work;\
    }\
    else if(Work->Thread_id==2){\
        Head->Next = Work;\
        Work->Prev = Head;\
        Current = Work;\
    }\
    else{\
        Work->Prev = Current;\
        Work->Next = Head;\
        Current->Next = Work;\
        Head->Prev = Work;\
    }\
    if(setjmp(Work->Environment)==0){\
        return;\
    }\
}

// Call this while a thread is terminated
#define ThreadExit() \
{\
	longjmp(SCHEDULER,2);\
}

// Decided whether to "context switch" based on the switchmode argument passed in main.c
#define ThreadYield() \
{\
    if(switchmode==0){\
        if(Work->Next!=Work){\
            longjmp(SCHEDULER,1);\
        }\
    }\
}
