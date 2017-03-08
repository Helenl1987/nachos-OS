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

#include "stdlib.h"
#include "unistd.h"
#include "sys/stat.h"
#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "sysdep.h"

extern StartProcess(char *filename);

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

struct SF {
	AddrSpace *space;
	int PC;
};

void fork_func(int s) {
	SF *sf = (SF*)s;
	currentThread->space = sf->space;
	int PC = sf->PC;
	machine->WriteRegister(PCReg, PC);
	machine->WriteRegister(NextPCReg, PC+4);
	currentThread->SaveUserState();
	printf("In fork_func::set user space, set PC, save user state, run!...\n");
	machine->Run();
}

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);
    if ((which == SyscallException) && (type == SC_Halt)) {
		printf("Syscall: Halt\n");
		DEBUG('a', "Shutdown, initiated by user program.\n");
 	  	interrupt->Halt();
 	  	return;
    } 
	
	if((which == SyscallException) && (type == SC_Exit)) {
		printf("Syscall: Exit\n");
		printf("%s start to exit...usedTimeSlices = %d\n", currentThread->getName(),currentThread->getusedTimeSlices());
		int mbmtmp = machine->MemoryBitmap->NumClear();
		printf("present MemoryBitmap: total %d used %d use rate %lf\n", NumPhysPages, NumPhysPages-mbmtmp, 1-(double)mbmtmp/NumPhysPages);
		for(int i = 0; i < NumPhysPages; i++) {
			if(machine->MemoryBitmap->Test(i) && machine->MemoryBitmap->mapentry[i].tid == currentThread->getThreadID()) {
				machine->MemoryBitmap->Clear(i);
				printf("release physical page %d for %s...\n", i, currentThread->getName());
			}
		}
		/*for(int i = 0; i < machine->pageTableSize; i++) {
			if(machine->MemoryBitmap->Test(machine->pageTable[i].physicalPage) && machine->MemoryBitmap->mapentry[machine->pageTable[i].physicalPage].tid == currentThread->getThreadID()) {
				machine->MemoryBitmap->Clear(machine->pageTable[i].physicalPage);
				printf("release physical page %d for %s...\n", machine->pageTable[i].physicalPage, currentThread->getName());
			}
		}*/
		mbmtmp = machine->MemoryBitmap->NumClear();
		printf("present MemoryBitmap: total %d used %d use rate %lf\n", NumPhysPages, NumPhysPages-mbmtmp, 1-(double)mbmtmp/NumPhysPages);
		if(currentThread->father != NULL) {
			for(int i = 0; i < 10; i++) {
				if(currentThread->father->child[i] == currentThread) {
					currentThread->father->child[i] = NULL;
					printf("thread %s child %d exit...\n", currentThread->father->getName(), i);
					break;
				}
			}
		}
		else {
			//machine->PCgo();
		}
		currentThread->Finish();
		return;
	}

	if((which == SyscallException) && (type == SC_Exec)) {
		printf("Syscall: Exec\n");
		int base = machine->ReadRegister(4);
		int value;
		int i;
		for(i = 0; ; i++) {
			machine->ReadMem(base+i, 1, &value);
			if(value == 0)
				break;
		}
		char filename[i];
		printf("File Name Length = %d\n", i);
		for(int j = 0; j < i; j++) {
			machine->ReadMem(base+j, 1, &value);
			filename[j] = (char)value;
		}
		filename[i] = 0;
		filename[12] = 0;
		printf("File Name = %s\n", filename);
		Thread *newthread = new Thread("newthread");
		for(i = 0; i < 10; i++) {
			if(currentThread->child[i] == NULL) {
				currentThread->child[i] = newthread;
				machine->WriteRegister(2, (int)newthread);
				break;
			}
		}
		if(i == 10) {
			printf("current Thread already has 10 children, Exec failed!\n");
		}
		else {
			printf("Exec file %s currentThread->child[%d] = newthread\n", filename, i);
			newthread->father = currentThread;
			newthread->Fork(StartProcess, filename);
			currentThread->Yield();
		}
		machine->PCgo();
		return;
	}

	if((which == SyscallException) && (type == SC_Join)) {
		printf("Syscall: Join\n");
		int id = machine->ReadRegister(4);
		Thread *thread = (Thread*)id;
		int num;
		int i;
		for(i = 0; i < 10; i++) {
			if(currentThread->child[i] == thread) {
				num = i;
				break;
			}
		}
		if(i == 10) {
			printf("Cannot find the thread, Join failed!\n");
			machine->PCgo();
			return;
		}
		while(currentThread->child[num] != NULL) {
			//printf("Thread %s is waiting for its child thread %s\n", currentThread->getName(), thread->getName());
			currentThread->Yield();
		}
		printf("Join succeed!\n");
		machine->PCgo();
		return;
	}

	if((which == SyscallException) && (type == SC_Create)) {
		printf("Syscall: Create\n");
		int namebase = machine->ReadRegister(4);
		int value;
		int i = 0;
		for(i = 0; ; i++) {
			machine->ReadMem(namebase+i, 1, &value);
			if(value == 0)
				break;
		}
		printf("File Name Length: %d\n", i);
		char filename[i];
		for(int j = 0; j < i; j++) {
			machine->ReadMem(namebase+j, 1, &value);
			filename[j] = (char)value;
		}
		filename[i] = 0;
		printf("filename = %s\n", filename);
		bool pan = fileSystem->Create(filename, 100);
		if(pan)
			printf("Create %s successful!\n", filename);
		else
			printf("Create %s failed!\n", filename);
		machine->PCgo();
		return;
	}

	if((which == SyscallException) && (type == SC_Open)) {
#ifdef FILESYS_STUB
		printf("Syscall: Open\n");
		int namebase = machine->ReadRegister(4);
		int value;
		int i = 0;
		for(i = 0; ; i++) {
			machine->ReadMem(namebase+i, 1, &value);
			if(value == 0)
				break;
		}
		printf("File Name Length: %d\n", i);
		char filename[i];
		for(int j = 0; j < i; j++) {
			machine->ReadMem(namebase+j, 1, &value);
			filename[j] = (char)value;
		}
		printf("filename = %s\n", filename);
		OpenFile *openfile = fileSystem->Open(filename);
		if(openfile == NULL) {
			printf("Open %s failed!\n", filename);
			machine->WriteRegister(2, -1);
		}
		else {
			printf("Open %s successful! fileID = %d\n", filename, openfile->getFileID());
			machine->WriteRegister(2, openfile->getFileID());
		}
#endif
		machine->PCgo();
		return;
	}

	if((which == SyscallException) && (type == SC_Read)) {
		//printf("Syscall: Read\n");
		int bufferbase = machine->ReadRegister(4);
		int size = machine->ReadRegister(5);
		int fileID = machine->ReadRegister(6);
		//printf("fileID = %d\n", fileID);
		if(fileID == 0) {
			char tmp[size];
			for(int i = 0; i < size; i++) {
				tmp[i] = getchar();
			}
			tmp[size] = 0;
			//printf("tmp = %s\n", tmp);
			for(int i = 0; i < size; i++) {
				machine->WriteMem(bufferbase+i, 1, tmp[i]);
			}
			//printf("Reading %d characters from file %d successfully!\n", size, fileID);
			machine->PCgo();
			return;
		}
		int numRead;
		OpenFile *openfile = new OpenFile(fileID);
		if(openfile == NULL) {
			printf("Open File %s failed!\n", fileID);
		}
		else {
				char tmp[size];
				numRead = openfile->Read(tmp, size);
				for(int i = 0; i < numRead; i++) {
					machine->WriteMem(bufferbase+i, 1, tmp[i]);
				}
				printf("Reading %d characters from file %d successfully!\n", numRead, fileID);
				delete openfile;
		}
		machine->PCgo();
		return;
	}

	if((which == SyscallException) && (type == SC_Write)) {
#ifdef FILESYS_STUB
		//printf("Syscall: Write\n");
		int bufferbase = machine->ReadRegister(4);
		int size = machine->ReadRegister(5);
		int fileID = machine->ReadRegister(6);
		int value;
		int i = 0;
		for(i = 0; ; i++) {
			machine->ReadMem(bufferbase+i, 1, &value);
			if(value == 0)
				break;
		}
		char buffer[i] = {};
		for(int j = 0; j < i; j++) {
			machine->ReadMem(bufferbase+j, 1, &value);
			buffer[j] = (char)value;
		}
		buffer[i] = 0;
		//printf("Write Content Length: %d, Content: %s\n", i, buffer);

		//printf("fileID = %d\n", fileID);
		if(fileID == 1) {
			//printf("hahaha");
			printf("%s", buffer);
			fflush(stdout);
			//printf("%s", buffer);
		}
		else {
			OpenFile *openfile = new OpenFile(fileID);
			printf("fileID = %d\n", openfile->getFileID());
			int numWrite = openfile->Write(buffer, size);
			//printf("haha\n");
			if(numWrite == 0) {
				printf("Writing file %d failed!\n", fileID);
			}
			else {
				//printf("Writing %d characters into file %d successfully!\n", numWrite, fileID);
			}
			delete openfile;
		}

#endif
		machine->PCgo();
		return;
	}

	if((which == SyscallException) && (type == SC_Close)) {
		//printf("Syscall: Close\n");
		int fileID = machine->ReadRegister(4);
		Close(fileID);
		//printf("Close file %d\n", fileID);
		machine->PCgo();
		return;
	}

	if((which == SyscallException) && (type == SC_Fork)) {
		printf("Syscall: Fork\n");
		int PC = machine->ReadRegister(4);
		struct SF *sf = new SF();
		sf->space = currentThread->space;
		sf->PC = PC;
		Thread *newthread = new Thread("forked thread");
		newthread->Fork(fork_func, (int)sf);
		machine->PCgo(); 
		return;
	}

	if((which == SyscallException) && (type == SC_Yield)) {
		printf("Syscall: Yield\n");
		machine->PCgo();
		printf("yielding thread %s\n", currentThread->getName());
		currentThread->Yield();
		return;
	}
	
	if(which == SyscallException && type == SC_Pwd) {
		//printf("Syscall: Pwd\n");
		system("pwd");
		machine->PCgo();
		return;
	}

	if(which == SyscallException && type == SC_Ls) {
		//printf("Syscall: Ls\n");
		system("ls");
		machine->PCgo();
		return;
	}

	if(which == SyscallException && type == SC_Cd) {
		//printf("Syscall: Cd\n");
		int base = machine->ReadRegister(4);
		int i;
		int value;
		for(i = 0; ; i++) {
			machine->ReadMem(base+i, 1, &value);
			if(value == 0)
				break;
		}
		char path[i];
		for(int j = 0; j < i; j++) {
			machine->ReadMem(base+j, 1, &value);
			path[j] = (char)value;
		}
		path[i] = 0;
		chdir(path);
		machine->PCgo();
		return;
	}

	if((which == SyscallException) && (type == SC_Remove)) {
		printf("Syscall: Remove\n");
		int namebase = machine->ReadRegister(4);
		int value;
		int i = 0;
		for(i = 0; ; i++) {
			machine->ReadMem(namebase+i, 1, &value);
			if(value == 0)
				break;
		}
		printf("File Name Length: %d\n", i);
		char filename[i];
		for(int j = 0; j < i; j++) {
			machine->ReadMem(namebase+j, 1, &value);
			filename[j] = (char)value;
		}
		filename[i] = 0;
		printf("filename = %s\n", filename);
		bool pan = fileSystem->Remove(filename);
		if(pan)
			printf("Remove %s successful!\n", filename);
		else
			printf("Remove %s failed!\n", filename);
		machine->PCgo();
		return;
	}

	if((which == SyscallException) && (type == SC_CreateDir)) {
		printf("Syscall: CreateDir\n");
		int namebase = machine->ReadRegister(4);
		int value;
		int i = 0;
		for(i = 0; ; i++) {
			machine->ReadMem(namebase+i, 1, &value);
			if(value == 0)
				break;
		}
		printf("File Name Length: %d\n", i);
		char path[i];
		for(int j = 0; j < i; j++) {
			machine->ReadMem(namebase+j, 1, &value);
			path[j] = (char)value;
		}
		path[i] = 0;
		printf("path = %s\n", path);
		mkdir(path, S_IRWXU);
		machine->PCgo();
		return;
	}

	if((which == SyscallException) && (type == SC_RemoveDir)) {
		printf("Syscall: RemoveDir\n");
		int namebase = machine->ReadRegister(4);
		int value;
		int i = 0;
		for(i = 0; ; i++) {
			machine->ReadMem(namebase+i, 1, &value);
			if(value == 0)
				break;
		}
		printf("File Name Length: %d\n", i);
		char path[i];
		for(int j = 0; j < i; j++) {
			machine->ReadMem(namebase+j, 1, &value);
			path[j] = (char)value;
		}
		path[i] = 0;
		printf("path = %s\n", path);
		rmdir(path);
		machine->PCgo();
		return;
	}

	/*#ifdef USE_TLB
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
	#endif*/

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
			//outthread->space->pageTable[machine->MemoryBitmap->mapentry[intoindex].vpn].valid = FALSE;
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
		//printf("copy disk page %d to mainmemory page %d belong to thread %d\n", fromindex, intoindex, tid);
		int mbmtmp = machine->MemoryBitmap->NumClear();
		//printf("present MemoryBitmap: total %d used %d use rate %lf\n", NumPhysPages, NumPhysPages-mbmtmp, 1-(double)mbmtmp/NumPhysPages);
		machine->MemoryBitmap->mapentry[intoindex].tid = tid;
		machine->MemoryBitmap->mapentry[intoindex].vpn = vpn;
		machine->MemoryBitmap->mapentry[intoindex].lru = stats->totalTicks;
		machine->MemoryBitmap->mapentry[intoindex].dirty = FALSE;
		machine->MemoryBitmap->mapentry[intoindex].use = FALSE;

		//currentThread->space->pageTable[vpn].valid = TRUE;
		//currentThread->space->pageTable[vpn].physicalPage = intoindex;
		return;
	}




	printf("Unexpected user mode exception %d %d\n", which, type);
	ASSERT(FALSE);


}













