#include "synch.h"
#include "thread.h"

#define N 20
#define Producer_Count 4
#define Consumer_Count 6

Lock* mutex;
Semaphore *empty;
Semaphore *full;
int empty_num = N;
int full_num = 0;
Thread *pcthread1[Consumer_Count+Producer_Count];

void *producer1(void *arg) {
	int id = (int)arg;
	for(int i = 0; i < 10; i++) {
		empty->P();
		mutex->Acquire();
		empty_num--;
		full_num++;
		printf("Producer %d produced an item, empty = %d, full = %d\n", id, empty_num, full_num);
		mutex->Release();
		full->V();
	}
}

void *consumer1(void *arg) {
	int id = (int)arg;
	for(int i = 0; i < 10; i++) {
		full->P();
		mutex->Acquire();
		full_num--;
		empty_num++;
		printf("Consumer %d consumed an item, empty = %d, full = %d\n", id, empty_num, full_num);
		mutex->Release();
		empty->V();
	}
}

void procon1Test() {
	mutex = new Lock("procon1 mutex");
	empty = new Semaphore("empty", N);
	full = new Semaphore("full", 0);

	for(int i = 0; i < Producer_Count; i++) {
		pcthread1[i] = new Thread("producer thread");
	}
	for(int i = 0; i < Consumer_Count; i++) {
		pcthread1[i+Producer_Count] = new Thread("consumer thread");
	}
	for(int i = 0; i < Producer_Count; i++) {
		pcthread1[i]->Fork(producer1, pcthread1[i]->getThreadID());
	}
	for(int i = 0; i < Consumer_Count; i++) {
		pcthread1[i+Producer_Count]->Fork(consumer1, pcthread1[i+Producer_Count]->getThreadID());
	}
}






