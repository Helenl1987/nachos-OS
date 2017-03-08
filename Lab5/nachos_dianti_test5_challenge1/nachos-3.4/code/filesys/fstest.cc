// fstest.cc 
//	Simple test routines for the file system.  
//
//	We implement:
//	   Copy -- copy a file from UNIX to Nachos
//	   Print -- cat the contents of a Nachos file 
//	   Perftest -- a stress test for the Nachos file system
//		read and write a really large file in tiny chunks
//		(won't work on baseline system!)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "utility.h"
#include "filesys.h"
#include "system.h"
#include "thread.h"
#include "disk.h"
#include "stats.h"

#define TransferSize 	10 	// make it small, just to be difficult

//----------------------------------------------------------------------
// Copy
// 	Copy the contents of the UNIX file "from" to the Nachos file "to"
//----------------------------------------------------------------------

void
Copy(char *from, char *to)
{
    FILE *fp;
    OpenFile* openFile;
    int amountRead, fileLength;
    char *buffer;

// Open UNIX file
    if ((fp = fopen(from, "r")) == NULL) {	 
	printf("Copy: couldn't open input file %s\n", from);
	return;
    }

// Figure out length of UNIX file
    fseek(fp, 0, 2);		
    fileLength = ftell(fp);
    fseek(fp, 0, 0);

// Create a Nachos file of the same length
    DEBUG('f', "Copying file %s, size %d, to file %s\n", from, fileLength, to);
    if (!fileSystem->Create(to, fileLength)) {	 // Create Nachos file
	printf("Copy: couldn't create output file %s\n", to);
	fclose(fp);
	return;
    }
    
    openFile = fileSystem->Open(to);
    ASSERT(openFile != NULL);
    
// Copy the data in TransferSize chunks
    buffer = new char[TransferSize];
    while ((amountRead = fread(buffer, sizeof(char), TransferSize, fp)) > 0)
	openFile->Write(buffer, amountRead);	
    delete [] buffer;

// Close the UNIX and the Nachos files
    delete openFile;
    fclose(fp);
}

//----------------------------------------------------------------------
// Print
// 	Print the contents of the Nachos file "name".
//----------------------------------------------------------------------

void
Print(char *name)
{
    OpenFile *openFile;    
    int i, amountRead;
    char *buffer;

    if ((openFile = fileSystem->Open(name)) == NULL) {
	printf("Print: unable to open file %s\n", name);
	return;
    }
    
    buffer = new char[TransferSize];
    while ((amountRead = openFile->Read(buffer, TransferSize)) > 0)
	for (i = 0; i < amountRead; i++)
	    printf("%c", buffer[i]);
    delete [] buffer;

    delete openFile;		// close the Nachos file
    return;
}

//----------------------------------------------------------------------
// PerformanceTest
// 	Stress the Nachos file system by creating a large file, writing
//	it out a bit at a time, reading it back a bit at a time, and then
//	deleting the file.
//
//	Implemented as three separate routines:
//	  FileWrite -- write the file
//	  FileRead -- read the file
//	  PerformanceTest -- overall control, and print out performance #'s
//----------------------------------------------------------------------

#define FileName 	"TestFile"
#define Contents 	"1234567890"
#define ContentSize 	strlen(Contents)
//#define FileSize 	((int)(ContentSize * 5000))
//#define FileSize 	((int)(ContentSize * 800))
#define FileSize 	((int)(ContentSize * 200))

#define name "root\/Dir1\/SubDir1\/TestFile"
#define path "root\/Dir1\/SubDir1\/"

static void 
FileWrite()
{
    OpenFile *openFile;    
    int i, numBytes;

    //printf("Sequential write of %d byte file, in %d byte chunks\n", FileSize, ContentSize);
	//printf("coming into FileWrite...\n");

    currentThread->openFile = fileSystem->Open(name, path);
    if (currentThread->openFile == NULL) {
		printf("Perf test: unable to open %s\n", FileName);
		return;
    }
	//printf("Perf test: open %s success\n", FileName);
    for (i = 0; i < FileSize; i += ContentSize) {
        numBytes = currentThread->openFile->Write(Contents, ContentSize);
		//printf("Writing %d bytes into file %s, nowpos = %d filelength = %d\n", numBytes, FileName, i, currentThread->openFile->Length());
		if (numBytes < 10) {
	    	printf("Perf test: unable to write %s\n", FileName);
	    	delete openFile;
	    	return;
		}
    }
	printf("Successfully write %d byte file, currentThread = %s\n", i, currentThread->getName());
//	fileSystem->Print();
    delete currentThread->openFile;	// close file
}

static void 
FileRead()
{
    OpenFile *openFile;    
    char *buffer = new char[ContentSize];
    int i, numBytes;

    //printf("Sequential read of %d byte file, in %d byte chunks, currentThread = %s\n", FileSize, ContentSize, currentThread->getName());

    //if ((openFile = fileSystem->Open(FileName)) == NULL) {
    if ((currentThread->openFile = fileSystem->Open(name, path)) == NULL) {
		printf("Perf test: unable to open file %s\n", FileName);
		delete [] buffer;
		return;
    }
	//printf("Perf test: open %s success\n", FileName);
    for (i = 0; i < FileSize; i += ContentSize) {
        numBytes = currentThread->openFile->Read(buffer, ContentSize);
		if ((numBytes < 10) || strncmp(buffer, Contents, ContentSize)) {
	    	printf("Perf test: unable to read %s\n", FileName);
	    	delete openFile;
	    	delete [] buffer;
	    	return;
		}
		//printf("Thread %s Reading %d bytes from %s file, nowpos = %d, FileSize = %d\n", currentThread->getName(), numBytes, FileName, i, FileSize);
		//printf("Content: %s\n", buffer);
    }
	printf("Successfully read %d byte file, currentThread = %s\n", i, currentThread->getName());
    delete [] buffer;
    delete currentThread->openFile;	// close file
}

void FuncFileRead(int n) {
	for(int i = 0; i < n; i++) {
		FileRead();
	}
}

void FuncFileWrite(int n) {
	for(int i = 0; i < n; i++) {
		FileWrite();
	}
}

void InitCreate() {
	fileSystem->CreateDir("root\/Dir1\/", "root\/");
	fileSystem->CreateDir("root\/Dir1\/SubDir1\/", "root\/Dir1\/");
	fileSystem->CreateDir("root\/Dir2\/", "root\/");

	printf("ContentSize = %d FileSize = %d\n", ContentSize, FileSize);
    //stats->Print();

    //if (!fileSystem->Create(FileName, 0)) {
    //if (!fileSystem->Create(FileName, FileSize)) {
    if (!fileSystem->Create(name, path, FileSize)) {
    	printf("Perf test: can't create %s\n", FileName);
    	return;
    }
	printf("Perf test: create %s success\n", FileName);
    //openFile = fileSystem->Open(FileName);
	//fileSystem->Print();
	//if(!fileSystem->AddLength(name, path, FileSize/2)) {
	//	printf("Perf test: add length to file %s failed\n", FileName);
	//	return;
	//}
}

void PerTestA() {
	printf("Starting FileWrite...\n");
    FileWrite();
	printf("Finishing FileWrite...\n");


	Thread *t1 = new Thread("forked thread 1");
	Thread *t2 = new Thread("forked thread 2");

	//printf("Starting FileRead...\n");
    //FileRead();
	//printf("Finishing FileRead...\n");
	t1->Fork(FileRead, NULL);
	t2->Fork(FileRead, NULL);
    //if (!fileSystem->Remove(FileName)) {
    if (!fileSystem->Remove(name, path)) {
      printf("Perf test: unable to remove %s\n", FileName);
      return;
    }
    stats->Print();
}

void PerTestB() {
	Thread *t1 = new Thread("forked thread 1");
	Thread *t2 = new Thread("forked thread 2");

	t1->Fork(FuncFileRead, 10);
	t2->Fork(FuncFileWrite, 10);

//    if (!fileSystem->Remove(name, path)) {
//      printf("Perf test: unable to remove %s\n", FileName);
//      return;
//    }
//    stats->Print();
}

void PerTestC() {
    OpenFile *openFile1;    
    if ((openFile1 = fileSystem->Open(name, path)) == NULL) {
		printf("Perf test: unable to open file %s\n", FileName);
		return;
    }
	printf("Perf test: open1 %s success\n", FileName);

    OpenFile *openFile2;    
    if ((openFile2 = fileSystem->Open(name, path)) == NULL) {
		printf("Perf test: unable to open file %s\n", FileName);
		return;
    }
	printf("Perf test: open2 %s success\n", FileName);
	
    if (!fileSystem->Remove(name, path)) {
		printf("Perf test1: unable to remove %s\n", FileName);
    }
	else {
		printf("Perf test1: remove successful\n");
	}

	delete openFile1;
    if (!fileSystem->Remove(name, path)) {
		printf("Perf test2: unable to remove %s\n", FileName);
    }
	else {
		printf("Perf test2: remove successful\n");
	}

	delete openFile2;
    if (!fileSystem->Remove(name, path)) {
		printf("Perf test3: unable to remove %s\n", FileName);
    }
	else {
		printf("Perf test3: remove successful\n");
	}
}

void
PerformanceTest()
{
    printf("Starting file system performance test, currentThread = %s\n", currentThread->getName());

	//InitCreate();
	//PerTestA();
	//PerTestB();
	//PerTestC();

    if (!fileSystem->Create("TestFile1", FileSize)) {
    	printf("Perf test: can't create %s\n", "TestFile1");
    	return;
    }
	printf("Perf test: create %s success\n", "TestFile1");

    if (!fileSystem->Create("TestFile2", FileSize)) {
    	printf("Perf test: can't create %s\n", "TestFile2");
    	return;
    }
	printf("Perf test: create %s success\n", "TestFile2");


}

