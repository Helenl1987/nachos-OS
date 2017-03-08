// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

void TLBMissFIFO(TranslationEntry* entry) {
    for(int i = 0; i < TLBSize-1; i++) {
    	machine->tlb[i] = machine->tlb[i+1];
    }
    machine->tlb[TLBSize-1].virtualPage = entry->virtualPage;
    machine->tlb[TLBSize-1].physicalPage = entry->physicalPage;
	machine->tlb[TLBSize-1].valid = entry->valid;
    machine->tlb[TLBSize-1].readOnly = entry->readOnly;
    machine->tlb[TLBSize-1].use = entry->use;
    machine->tlb[TLBSize-1].dirty = entry->dirty;
}

void TLBMissLRU(TranslationEntry* entry) {
	unsigned int tmp = machine->tlb[0].lru;
	int ind = 0;
	for(int i = 0; i < TLBSize; i++) {
		if(machine->tlb[i].lru < tmp) {
			tmp = machine->tlb[i].lru;
			ind = i;
		}
	}
    machine->tlb[ind].virtualPage = entry->virtualPage;
    machine->tlb[ind].physicalPage = entry->physicalPage;
    machine->tlb[ind].valid = entry->valid;
    machine->tlb[ind].readOnly = entry->readOnly;
    machine->tlb[ind].use = entry->use;
    machine->tlb[ind].dirty = entry->dirty;
	machine->tlb[ind].lru = stats->totalTicks;
}

void TLBMissLFU(TranslationEntry* entry) {
	int tmp = machine->tlb[0].lfu;
	int ind = 0;
	for(int i = 0; i < TLBSize; i++) {
		printf("i = %d, lfu = %d, tmp = %d\n",i,machine->tlb[i].lfu,tmp);
		if(machine->tlb[i].lfu < tmp) {
			tmp = machine->tlb[i].lfu;
			ind = i;
		}
	}
	printf("ind = %d\n",ind);
    machine->tlb[ind].virtualPage = entry->virtualPage;
    machine->tlb[ind].physicalPage = entry->physicalPage;
    machine->tlb[ind].valid = entry->valid;
    machine->tlb[ind].readOnly = entry->readOnly;
    machine->tlb[ind].use = entry->use;
    machine->tlb[ind].dirty = entry->dirty;
	machine->tlb[ind].lfu = 1;
}

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);
    if ((which == SyscallException) && (type == SC_Halt)) {
		DEBUG('a', "Shutdown, initiated by user program.\n");
 	  	interrupt->Halt();
 	  	return;
    } 
	
	if((which == SyscallException) && (type == SC_Exit)) {
		printf("%s start to exit...usedTimeSlices = %d\n", currentThread->getName(),currentThread->getusedTimeSlices());
		int mbmtmp = machine->MemoryBitmap->NumClear();
		printf("present MemoryBitmap: total %d used %d use rate %lf\n", NumPhysPages, NumPhysPages-mbmtmp, 1-(double)mbmtmp/NumPhysPages);
		for(int i = 0; i < machine->pageTableSize; i++) {
			if(machine->MemoryBitmap->Test(machine->pageTable[i].physicalPage) && machine->MemoryBitmap->mapentry[machine->pageTable[i].physicalPage].tid == currentThread->getThreadID()) {
				machine->MemoryBitmap->Clear(machine->pageTable[i].physicalPage);
				printf("release physical page %d for %s...\n", machine->pageTable[i].physicalPage, currentThread->getName());
			}
		}
		mbmtmp = machine->MemoryBitmap->NumClear();
		printf("present MemoryBitmap: total %d used %d use rate %lf\n", NumPhysPages, NumPhysPages-mbmtmp, 1-(double)mbmtmp/NumPhysPages);
		currentThread->Finish();
		return;
	}

    if(which == TLBMissException) {
    	//if(machine->tlb != NULL) {
			//printf("TLB miss...\n");
     		int virtAddr = machine->ReadRegister(BadVAddrReg);
    		unsigned int vpn = (unsigned) virtAddr / PageSize;
    		unsigned int offset = (unsigned) virtAddr % PageSize;
    		TranslationEntry *entry = &(machine->pageTable)[vpn];
    		for(int i = 0; i < TLBSize; i++) {
    			if(machine->tlb[i].valid == FALSE) {
    				machine->tlb[i].virtualPage = entry->virtualPage;
    				machine->tlb[i].physicalPage = entry->physicalPage;
    				machine->tlb[i].valid = entry->valid;
    				machine->tlb[i].readOnly = entry->readOnly;
    				machine->tlb[i].use = entry->use;
    				machine->tlb[i].dirty = entry->dirty;
    				return;
    			}
    		}
			//TLBMissFIFO(entry);
			TLBMissLRU(entry);
			//TLBMissLFU(entry);
    	//}
    	return;
    }

	if(which == PageFaultException) {
		/*if(currentThread->getusedTimeSlices() >= 20) {
			ASSERT(FALSE);
		}*/
		//printf("in page fault exception handler\n");
		int virtAddr = machine->ReadRegister(BadVAddrReg);
		unsigned int vpn = (unsigned) virtAddr / PageSize;
		unsigned int offset = (unsigned) virtAddr % PageSize;
		int tid = currentThread->getThreadID();
		int fromindex;
		for(int i = 0; i < NumDiskPages; i++) {
			if(machine->vdiskentry[i].tid == tid && machine->vdiskentry[i].vpn == vpn) {
				fromindex = i;
				break;
			}
		}
		int intoindex = machine->MemoryBitmap->Find();
		if(intoindex == -1) {
			//printf("##########main memory full\n");
			int tmp = machine->MemoryBitmap->mapentry[0].lru;
			intoindex = 0;
			for(int j = 0; j < NumPhysPages; j++) {
				//printf("j = %d lru = %d tmp = %d ",j,machine->MemoryBitmap->mapentry[j].lru,tmp);
				if(machine->MemoryBitmap->mapentry[j].lru < tmp) {
					tmp = machine->MemoryBitmap->mapentry[j].lru;
					intoindex = j;
				}
			}
			//printf("intoindex = %d\n",intoindex);
			Thread *outthread = threadpointer[machine->MemoryBitmap->mapentry[intoindex].tid];
			outthread->space->pageTable[machine->MemoryBitmap->mapentry[intoindex].vpn].valid = FALSE;
			if(machine->MemoryBitmap->mapentry[intoindex].dirty == TRUE) {
				//printf("$$$$$$$$$handle dirty\n");
				int outdiskwhere;
				for(int k = 0; k < NumDiskPages; k++) {
					if(machine->vdiskentry[k].tid == machine->MemoryBitmap->mapentry[intoindex].tid && machine->vdiskentry[k].vpn == machine->MemoryBitmap->mapentry[intoindex].vpn) {
						outdiskwhere = k;
						break;
					}
				}
				//printf("outdisk where = %d\n",outdiskwhere);
				bcopy(&(machine->mainMemory[intoindex*PageSize]), &(machine->vdisk[outdiskwhere*PageSize]), PageSize);
				printf("write dirty main memory page %d back to disk page %d\n",intoindex,outdiskwhere);
			}
		}
		//printf("virtAddr = %d vpn = %d tid = %d fromindex = %d intoindex = %d\n",virtAddr, vpn,tid,fromindex,intoindex);
		bcopy(&(machine->vdisk[fromindex*PageSize]), &(machine->mainMemory[intoindex*PageSize]), PageSize);
		printf("copy disk page %d to mainmemory page %d belong to thread %d\n", fromindex, intoindex, tid);
		int mbmtmp = machine->MemoryBitmap->NumClear();
		//printf("present MemoryBitmap: total %d used %d use rate %lf\n", NumPhysPages, NumPhysPages-mbmtmp, 1-(double)mbmtmp/NumPhysPages);
		machine->MemoryBitmap->mapentry[intoindex].tid = tid;
		machine->MemoryBitmap->mapentry[intoindex].vpn = vpn;
		machine->MemoryBitmap->mapentry[intoindex].lru = stats->totalTicks;
		machine->MemoryBitmap->mapentry[intoindex].dirty = FALSE;
		currentThread->space->pageTable[vpn].valid = TRUE;
		currentThread->space->pageTable[vpn].physicalPage = intoindex;
		return;
	}




	printf("Unexpected user mode exception %d %d\n", which, type);
	ASSERT(FALSE);


}













