#include "synch.h"
#include "thread.h"
#include "system.h"

//#define Reader_cnt 4
//#define Writer_cnt 6

class RWLock {
public:
	int reader_cnt;
	Lock *mutex;
	Lock *wlock;

	RWLock() {
		reader_cnt = 0;
		mutex = new Lock("rwlock mutex");
		wlock = new Lock("rwlock wlock");
	}

	void read(void *arg) {
		int id = (int)arg;
		mutex->Acquire();
		reader_cnt++;
		if(reader_cnt == 1){
			wlock->Acquire();
		}
		mutex->Release();

		printf("Reader %s id = %d reading the file\n", currentThread->getName(), id);

		mutex->Acquire();
		reader_cnt--;
		if(reader_cnt == 0) {
			wlock->Release();
		}
		mutex->Release();

	}

	void write(void *arg) {
		int id = (int)arg;
		wlock->Acquire();
		printf("Writer %s id = %d writing the file\n", currentThread->getName(), id);
		wlock->Release();

	}

};

RWLock *rwlock;

void *Reader(void * arg) {
	for(int i = 0; i < 5; i++)
		rwlock->read(arg);
}

void *Writer(void * arg) {
	for(int i = 0; i < 5; i++)
		rwlock->write(arg);
}


void RWLockTest() {
	rwlock = new RWLock();
	//Thread *rwthread[Reader_cnt + Writer_cnt];

	Thread *t1 = new Thread("reader thread 1");
	Thread *t2 = new Thread("writer thread 2");
	Thread *t3 = new Thread("reader thread 3");
	Thread *t4 = new Thread("reader thread 4");
	Thread *t5 = new Thread("writer thread 5");
	Thread *t6 = new Thread("writer thread 6");

	t1->Fork(Reader, t1->getThreadID());
	t2->Fork(Writer, t2->getThreadID());
	t3->Fork(Reader, t3->getThreadID());
	t4->Fork(Reader, t4->getThreadID());
	t5->Fork(Writer, t5->getThreadID());
	t6->Fork(Writer, t6->getThreadID());
}













