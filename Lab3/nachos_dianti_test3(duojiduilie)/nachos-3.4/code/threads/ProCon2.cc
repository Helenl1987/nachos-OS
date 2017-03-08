#include "synch.h"
#include "thread.h"

#define N 20
#define Producer_Count 4
#define Consumer_Count 6
Lock *pclock;
Condition *pccondition;
Thread *pcthread2[Consumer_Count+Producer_Count];
int cnt = 0;

void *producer2(void *arg) {
	int id = (int)arg;
	for(int i = 0; i < 10; i++) {
		pclock->Acquire();
		while(cnt == N) {
			pccondition->Wait(pclock);
		}
		cnt++;
		printf("Producer %d produced an item, cnt = %d\n", id, cnt);
		pccondition->Signal(pclock);
		pclock->Release();
	}
}

void *consumer2(void *arg) {
	int id = (int)arg;
	for(int i = 0; i < 10; i++) {
		pclock->Acquire();
		while(cnt == 0) {
			pccondition->Wait(pclock);
		}
		cnt--;
		printf("Consumer %d consumed an item, cnt = %d\n", id, cnt);
		pccondition->Signal(pclock);
		pclock->Release();
	}
}

void procon2Test() {
	pclock = new Lock("procon2 pclock");
	pccondition = new Condition("procon2 pccondition");
	for(int i = 0; i < Producer_Count; i++) {
		pcthread2[i] = new Thread("producer thread");
	}
	for(int i = 0; i < Consumer_Count; i++) {
		pcthread2[i+Producer_Count] = new Thread("consumer thread");
	}
	for(int i = 0; i < Producer_Count; i++) {
		pcthread2[i]->Fork(producer2, pcthread2[i]->getThreadID());
	}
	for(int i = 0; i < Consumer_Count; i++) {
		pcthread2[i+Producer_Count]->Fork(consumer2, pcthread2[i+Producer_Count]->getThreadID());
	}
}




