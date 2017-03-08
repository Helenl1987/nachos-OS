// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "elevatortest.h"

// testnum is set in main.cc
int testnum = 1;


void TS() {
	printf("Showing Thread Status...\n");
	for(int i = 0; i < 128; i++) {
		if(numthreadID[i] == 1) {
			threadpointer[i]->ShowThreadStatus();
		}
	}
}


//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

void
SimpleThread(int which)
{
    int num;
    
    for (num = 0; num < 5; num++) {
		printf("*** thread %d \"%s\" looped %d times priority = %d\n", currentThread->getThreadID(), currentThread->getName(), num, currentThread->getPriority());
		//TS();
        //currentThread->Yield();
    }
}

//----------------------------------------------------------------------
// ThreadTest1
// 	Set up a ping-pong between two threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest1()
{
    DEBUG('t', "Entering ThreadTest1");

	currentThread->setPriority(10);

    Thread *t1 = new Thread("forked thread1", 5);
    Thread *t2 = new Thread("forked thread2", 7);
    Thread *t3 = new Thread("forked thread3", 0);
    Thread *t4 = new Thread("forked thread4", 5);

    t1->Fork(SimpleThread, t1->getThreadID());
    t2->Fork(SimpleThread, t2->getThreadID());
    t3->Fork(SimpleThread, t3->getThreadID());
    t4->Fork(SimpleThread, t4->getThreadID());
	
	
	//TS();
   // SimpleThread(0);
}

//----------------------------------------------------------------------
// ThreadTest
// 	Invoke a test routine.
//----------------------------------------------------------------------

void
ThreadTest()
{
    switch (testnum) {
    case 1:
	ThreadTest1();
	break;
    default:
	printf("No test specified.\n");
	break;
    }
}

