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

extern void StartProcess(char *filename);
#ifdef USER_PROGRAM
void usertest(char *filename) {
	StartProcess(filename);
}

void UserThreadTest() {
	Thread *t1 = new Thread("forked user thread 1");
	Thread *t2 = new Thread("forked user thread 2");
	t1->Fork(usertest, "../test/matmult");
	t2->Fork(usertest, "../test/mamama");
}
#endif //USER_PROGRAM

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
	printf("*** thread %d looped %d times\n", which, num);
        currentThread->Yield();
    }
}

void
SimpleThread2(int which)
{
    int num;
    
    for (num = 0; num < 50; num++) {
	interrupt->SetLevel(IntOn);
	interrupt->SetLevel(IntOff);
	printf("*** thread %d \"%s\" looped %d times usedTimeSlices = %d\n", currentThread->getThreadID(), currentThread->getName(), num, currentThread->getusedTimeSlices());
	//TS();
        //currentThread->Yield();
	//scheduler->Print();

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

    Thread *t = new Thread("forked thread");

    t->Fork(SimpleThread, (void*)1);
    SimpleThread(0);
}

void
ThreadTest2()
{
    DEBUG('t', "Entering ThreadTest1");

    Thread *t1 = new Thread("forked thread1");
    Thread *t2 = new Thread("forked thread2");
    Thread *t3 = new Thread("forked thread3");

    t1->Fork(SimpleThread2, t1->getThreadID());
    t2->Fork(SimpleThread2, t2->getThreadID());
    t3->Fork(SimpleThread2, t3->getThreadID());
	
	TS();
    SimpleThread2(0);
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
	#ifdef USER_PROGRAM
		UserThreadTest();
	#endif //USER_PROGRAM
	#ifndef USER_PROGRAM
		//ThreadTest1();
		ThreadTest2();
	#endif
	break;
    default:
	printf("No test specified.\n");
	break;
    }
}

