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
#define FileSize 	((int)(ContentSize * 800))

static void 
FileWrite()
{
    OpenFile *openFile;    
    int i, numBytes;

	char *name = "root\/Dir1\/SubDir1\/TestFile";
	char *path = "root\/Dir1\/SubDir1\/";

    printf("Sequential write of %d byte file, in %d byte chunks\n", 
	FileSize, ContentSize);
	printf("coming into FileWrite...\n");
    //if (!fileSystem->Create(FileName, 0)) {
    //if (!fileSystem->Create(FileName, FileSize)) {
    if (!fileSystem->Create(name, path, FileSize/2)) {
    	printf("Perf test: can't create %s\n", FileName);
    	return;
    }
	printf("Perf test: create %s success\n", FileName);
    //openFile = fileSystem->Open(FileName);
	fileSystem->Print();
	if(!fileSystem->AddLength(name, path, FileSize/2)) {
		printf("Perf test: add length to file %s failed\n", FileName);
		return;
	}
    openFile = fileSystem->Open(name, path);
    if (openFile == NULL) {
		printf("Perf test: unable to open %s\n", FileName);
		return;
    }
	printf("Perf test: open %s success\n", FileName);
    for (i = 0; i < FileSize; i += ContentSize) {
        numBytes = openFile->Write(Contents, ContentSize);
		printf("Writing %d bytes into file %s, nowpos = %d filelength = %d\n", numBytes, FileName, i, openFile->Length());
		if (numBytes < 10) {
	    	printf("Perf test: unable to write %s\n", FileName);
	    	delete openFile;
	    	return;
		}
    }
//	fileSystem->Print();
    delete openFile;	// close file
}

static void 
FileRead()
{
    OpenFile *openFile;    
    char *buffer = new char[ContentSize];
    int i, numBytes;

	char *name = "root\/Dir1\/SubDir1\/TestFile";
	char *path = "root\/Dir1\/SubDir1\/";

    printf("Sequential read of %d byte file, in %d byte chunks\n", FileSize, ContentSize);

    //if ((openFile = fileSystem->Open(FileName)) == NULL) {
    if ((openFile = fileSystem->Open(name, path)) == NULL) {
		printf("Perf test: unable to open file %s\n", FileName);
		delete [] buffer;
		return;
    }
	printf("Perf test: open %s success\n", FileName);
    for (i = 0; i < FileSize; i += ContentSize) {
        numBytes = openFile->Read(buffer, ContentSize);
		if ((numBytes < 10) || strncmp(buffer, Contents, ContentSize)) {
	    	printf("Perf test: unable to read %s\n", FileName);
	    	delete openFile;
	    	delete [] buffer;
	    	return;
		}
		printf("Reading %d bytes from %s file, nowpos = %d, FileSize = %d\n", numBytes, FileName, i, FileSize);
		printf("Content: %s\n", buffer);
    }
    delete [] buffer;
    delete openFile;	// close file
}

void
PerformanceTest()
{
	char *name = "root\/Dir1\/SubDir1\/TestFile";
	char *path = "root\/Dir1\/SubDir1\/";

    printf("Starting file system performance test:\n");

	fileSystem->CreateDir("root\/Dir1\/", "root\/");
	fileSystem->CreateDir("root\/Dir1\/SubDir1\/", "root\/Dir1\/");
	fileSystem->CreateDir("root\/Dir2\/", "root\/");

	printf("ContentSize = %d FileSize = %d\n", ContentSize, FileSize);
    stats->Print();
	printf("Starting FileWrite...\n");
    FileWrite();
	printf("Finishing FileWrite...\n");
	printf("Starting FileRead...\n");
    FileRead();
	printf("Finishing FileRead...\n");
    //if (!fileSystem->Remove(FileName)) {
    if (!fileSystem->Remove(name, path)) {
      printf("Perf test: unable to remove %s\n", FileName);
      return;
    }
    stats->Print();


}

