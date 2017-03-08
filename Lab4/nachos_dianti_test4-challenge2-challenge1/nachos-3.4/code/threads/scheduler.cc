// scheduler.cc 
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would 
//	end up calling FindNextToRun(), and that would put us in an 
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "scheduler.h"
#include "system.h"


//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads to empty.
//----------------------------------------------------------------------

Scheduler::Scheduler()
{ 
    readyList1 = new List; 
    readyList2 = new List;
    readyList3 = new List;
	suspendlist = new List;

} 

//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

Scheduler::~Scheduler()
{ 
    delete readyList1; 
    delete readyList2;
    delete readyList3;
	delete suspendlist;
} 

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

void
Scheduler::ReadyToRun (Thread *thread)
{
    DEBUG('t', "Putting thread %s on ready list.\n", thread->getName());

    thread->setStatus(READY);
	//Print();

	int readynum = readyList1->NumInList() + readyList2->NumInList() + readyList3->NumInList();
	if(readynum >= MaxReadyThread) {
		Thread *tosuspend = NULL;
		tosuspend = (Thread*)readyList3->Remove();
		if(tosuspend == NULL) {
			tosuspend = (Thread*)readyList2->Remove();
			if(tosuspend == NULL) {
				tosuspend = (Thread*)readyList1->Remove();
				if(tosuspend->getusedTimeSlices() == 0) {
					readyList1->Append(tosuspend);
					tosuspend = NULL;
				}
			}
		}
		if(tosuspend == NULL) {
			thread->Suspend();
			return;
		}
		else 
			tosuspend->Suspend();
	}

    	if(thread->getusedTimeSlices() < 2) {
        	readyList1->Append((void *)thread);
        	if(thread != currentThread) {
            	if(currentThread->getusedTimeSlices() >= 2) {
                	currentThread->Yield();
            	}
        	}
    	}
    	else if(thread->getusedTimeSlices() < 6) {
        	readyList2->Append((void *)thread);
        	if(thread != currentThread) {
            	if(currentThread->getusedTimeSlices() >= 6) {
                	currentThread->Yield();
            	}
        	}
    	}
    	else
        	readyList3->Append((void *)thread);
		
		//Print();

}

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------

Thread *
Scheduler::FindNextToRun ()
{
	//printf("!!!@@@find next to run in %s\n",currentThread->getName());
	//Print();
	Thread *tmp;
    if(readyList1->IsEmpty()) {
        if(readyList2->IsEmpty()) {
            tmp = (Thread *)readyList3->Remove();
        }
        else
            tmp = (Thread *)readyList2->Remove();
    }
    else
        tmp = (Thread *)readyList1->Remove();

	int readynum = readyList1->NumInList() + readyList2->NumInList() + readyList3->NumInList();
	//Print();
	Thread *toactive = NULL;
	//printf("readynum = %d max = %d \n", readynum, MaxReadyThread);
	if(readynum+1 < MaxReadyThread) {
		toactive = suspendlist->Remove();
		if(toactive != NULL)
			toactive->Active();
	}
	return tmp;
}

//----------------------------------------------------------------------
// Scheduler::Run
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//----------------------------------------------------------------------

void
Scheduler::Run (Thread *nextThread)
{
    Thread *oldThread = currentThread;
    
#ifdef USER_PROGRAM			// ignore until running user programs 
    if (currentThread->space != NULL) {	// if this thread is a user program,
        currentThread->SaveUserState(); // save the user's CPU registers
	currentThread->space->SaveState();
    }
#endif
    
    oldThread->CheckOverflow();		    // check if the old thread
					    // had an undetected stack overflow

    currentThread = nextThread;		    // switch to the next thread
    currentThread->setStatus(RUNNING);      // nextThread is now running
    
    DEBUG('t', "Switching from thread \"%s\" to thread \"%s\"\n",
	  oldThread->getName(), nextThread->getName());
    
    // This is a machine-dependent assembly language routine defined 
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".

    SWITCH(oldThread, nextThread);
    
	//if(oldThread != nextThread)
		printf("switch to %s\n", currentThread->getName());

    DEBUG('t', "Now in thread \"%s\"\n", currentThread->getName());

    // If the old thread gave up the processor because it was finishing,
    // we need to delete its carcass.  Note we cannot delete the thread
    // before now (for example, in Thread::Finish()), because up to this
    // point, we were still running on the old thread's stack!
    if (threadToBeDestroyed != NULL) {
        delete threadToBeDestroyed;
	threadToBeDestroyed = NULL;
    }
    
#ifdef USER_PROGRAM
    if (currentThread->space != NULL) {		// if there is an address space
        currentThread->RestoreUserState();     // to restore, do it.
	currentThread->space->RestoreState();
    }
#endif
}

//----------------------------------------------------------------------
// Scheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------
void
Scheduler::Print()
{
    printf("Ready list 1 contents:\n");
    readyList1->Mapcar((VoidFunctionPtr) ThreadPrint);
	printf("\n");
    printf("Ready list 2 contents:\n");
    readyList2->Mapcar((VoidFunctionPtr) ThreadPrint);
	printf("\n");
    printf("Ready list 3 contents:\n");
    readyList3->Mapcar((VoidFunctionPtr) ThreadPrint);
	printf("\n");
}