#include "synch.h"
#include "thread.h"
#include "system.h"

#define TOTAL 3
Lock *block;
Condition *bcondition;
int bcnt = 0;

void barrier(void *arg) {
	int id = (int)arg;
	block->Acquire();
	bcnt++;
	if(bcnt == TOTAL) {
		printf("thread %s id = %d, cnt = %d, Broadcast...\n", currentThread->getName(), id, bcnt);
		bcondition->Broadcast(block);
		bcnt = 0;
		block->Release();
	}
	else {
		printf("thread %s id = %d, cnt = %d, Wait...\n", currentThread->getName(), id, bcnt);
		bcondition->Wait(block);
		printf("thread %s id = %d, cnt = %d, Wait successful, start running...\n", currentThread->getName(), id, bcnt);
		block->Release();
	}
}

void barrierTest() {
	block = new Lock("barrier block");
	bcondition = new Condition("barrier bcondition");
	Thread *t1 = new Thread("barrier thread 1");
	Thread *t2 = new Thread("barrier thread 2");
	Thread *t3 = new Thread("barrier thread 3");
	Thread *t4 = new Thread("barrier thread 4");
	Thread *t5 = new Thread("barrier thread 5");
	Thread *t6 = new Thread("barrier thread 6");

	t1->Fork(barrier, t1->getThreadID());
	t2->Fork(barrier, t2->getThreadID());
	t3->Fork(barrier, t3->getThreadID());
	t4->Fork(barrier, t4->getThreadID());
	t5->Fork(barrier, t5->getThreadID());
	t6->Fork(barrier, t6->getThreadID());
}
