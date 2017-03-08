// filehdr.cc 
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the 
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the 
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect 
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector, 
//
//      Unlike in a real system, we do not keep track of file permissions, 
//	ownership, last modification date, etc., in the file header. 
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "system.h"
#include "filehdr.h"

//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------

bool
FileHeader::Allocate(BitMap *freeMap, int fileSize)
{ 
    numBytes = fileSize;
	refercnt = 0;
    numSectors  = divRoundUp(fileSize, SectorSize);
	//printf("coming into Allocate...numSectors = %d\n", numSectors);
    if (freeMap->NumClear() < numSectors)
	return FALSE;		// not enough space

//    for (int i = 0; i < numSectors; i++)
//	   dataSectors[i] = freeMap->Find();

	if(numSectors <= NumDirect) {
		int pan = freeMap->Find_Complete(numSectors);
		if(pan == -1) {
			for(int i = 0; i < numSectors; i++)
				dataSectors[i] = freeMap->Find();
				//printf("Direct %d allocate sector %d\n", i, dataSectors[i]);
		}
		else {
			for(int i = 0; i < numSectors; i++) {
				dataSectors[i] = freeMap->Find_ByTrack(pan);
				ASSERT(dataSectors[i] != -1);
				//printf("Direct %d allocate sector %d\n", i, dataSectors[i]);
			}
		}
	}
	else {
		//printf("Starting Indirect index...\n");
		for(int i = 0; i < NumDirect; i++) {
			dataSectors[i] = freeMap->Find();
			//printf("Direct %d allocate sector %d\n", i, dataSectors[i]);
		}
		int restSectors = numSectors - NumDirect;
		int indirect = divRoundUp(restSectors, IndirectSize/SectorSize);
		//printf("direct = %d indirect = %d indirectSectors = %d\n", NumDirect, restSectors, indirect);
		for(int i = 0; i < indirect; i++) {
//			printf("hahaha i = %d\n", i);
			indexSectors[i] = freeMap->Find();
			int nowSectorSize = IndirectSize/SectorSize;
			if(i == indirect-1)
				nowSectorSize = restSectors - (indirect-1) * (IndirectSize/SectorSize);
			//printf("Indirect %d allocate sector %d nowSectorSize = %d\n", i, indexSectors[i], nowSectorSize);
			int *nowSector = new int[nowSectorSize];
			for(int j = 0; j < nowSectorSize; j++) {
				nowSector[j] = freeMap->Find();
				//printf("Indirect %d in sector %d, index %d allocate sector %d\n", i, indexSectors[i], j, nowSector[j]);
			}
//			printf("check ## sectornum = %d\n", indexSectors[i]);
			synchDisk->WriteSector(indexSectors[i], (char*)nowSector);
//			printf("lalala i = %d\n", i);
			delete nowSector;

		}
	}
	//printf("Allocate success!\n");
    createTime = stats->totalTicks;
    lastVisitTime = stats->totalTicks;
    lastModifyTime = stats->totalTicks;

    return TRUE;
}


bool FileHeader::ExtraAllocate(BitMap *freeMap, int newSize) {
	int newsector = divRoundUp(newSize, SectorSize);
	int oldSize = FileLength();
	int oldsector = divRoundUp(oldSize, SectorSize);
	if(freeMap->NumClear() < newsector) {
		printf("no more space for new size\n");
		return FALSE;
	}
	int totalsector = oldsector + newsector;
	if(totalsector * SectorSize > MaxFileSize) {
		printf("file size exceed MaxFileSize!\n");
		return FALSE;
	}

	numBytes = numBytes + newSize;
	numSectors = totalsector;
	lastVisitTime = stats->totalTicks;
    lastModifyTime = stats->totalTicks;

	if(totalsector <= NumDirect) {
		printf("ExtraAllocate: just use direct index\n");
		for(int i = 0; i < newsector; i++) {
			dataSectors[oldsector+i] = freeMap->Find();
			printf("Extra: dataSectors[%d] = %d\n", oldsector+i, dataSectors[oldsector+i]);
		}
	}
	else if(oldsector < NumDirect) {
		printf("ExtraAllocate: need direct index and indirect index\n");
		int direct_remain_sector = NumDirect - oldsector;
		for(int i = 0; i < direct_remain_sector; i++) {
			dataSectors[oldsector+i] = freeMap->Find();
			printf("Extra: dataSectors[%d] = %d\n", oldsector+i, dataSectors[oldsector+i]);
		}
		int restSectors = newsector - direct_remain_sector;
		int indirect = divRoundUp(restSectors, IndirectSize/SectorSize);
		printf("Extra: direct = %d indirect = %d indirectSectors = %d\n", direct_remain_sector, restSectors, indirect);
		for(int i = 0; i < indirect; i++) {
//			printf("hahaha i = %d\n", i);
			indexSectors[i] = freeMap->Find();
			int nowSectorSize = IndirectSize/SectorSize;
			if(i == indirect-1)
				nowSectorSize = restSectors - (indirect-1) * (IndirectSize/SectorSize);
			printf("Extra: Indirect %d allocate sector %d nowSectorSize = %d\n", i, indexSectors[i], nowSectorSize);
			int *nowSector = new int[nowSectorSize];
			for(int j = 0; j < nowSectorSize; j++) {
				nowSector[j] = freeMap->Find();
				printf("Extra: Indirect %d in sector %d, index %d allocate sector %d\n", i, indexSectors[i], j, nowSector[j]);
			}
//			printf("check ## sectornum = %d\n", indexSectors[i]);
			synchDisk->WriteSector(indexSectors[i], (char*)nowSector);
//			printf("lalala i = %d\n", i);
			delete nowSector;

		}
	}
	else {
		printf("ExtraAllocate: just use indirect index\n");
		int oldindirect = divRoundUp(oldsector-NumDirect, IndirectSize/SectorSize);
		int old_lastsector_num = oldsector - NumDirect - (oldindirect-1) * (IndirectSize/SectorSize);
		printf("Extra: oldindirect = %d old_lastsector_num = %d\n", oldindirect, old_lastsector_num);
		int *old_lastsector = new int[IndirectSize/SectorSize];
		synchDisk->ReadSector(indexSectors[oldindirect-1], (char*)old_lastsector);
		if(old_lastsector_num + newsector <= (IndirectSize/SectorSize)) {
			for(int i = 0; i < newsector; i++) {
				old_lastsector[old_lastsector_num+i] = freeMap->Find();
			}
			synchDisk->WriteSector(indexSectors[oldindirect-1], (char*)old_lastsector);
		}
		else {
			for(int i = 0; i+old_lastsector_num < (IndirectSize/SectorSize); i++) {
				old_lastsector[old_lastsector_num+i] = freeMap->Find();
				printf("Extra: Align: Indirect %d in sector %d, index %d allocate sector %d\n", oldindirect-1, indexSectors[oldindirect-1], old_lastsector_num+i, old_lastsector[old_lastsector_num+i]);
			}
			synchDisk->WriteSector(indexSectors[oldindirect-1], (char*)old_lastsector);

			int restSectors = newsector - ((IndirectSize/SectorSize)-old_lastsector_num);
			int indirect = divRoundUp(restSectors, IndirectSize/SectorSize);
			printf("Extra: indirect_align = %d indirect_notalign = %d complete indirectSectors = %d\n", newsector-restSectors, restSectors, indirect);
			for(int i = 0; i < indirect; i++) {
//				printf("hahaha i = %d\n", i);
				indexSectors[oldindirect+i] = freeMap->Find();
				int nowSectorSize = IndirectSize/SectorSize;
				if(i == indirect-1)
					nowSectorSize = restSectors - (indirect-1) * (IndirectSize/SectorSize);
				printf("Extra: Indirect %d allocate sector %d nowSectorSize = %d\n", oldindirect+i, indexSectors[oldindirect+i], nowSectorSize);
				int *nowSector = new int[nowSectorSize];
				for(int j = 0; j < nowSectorSize; j++) {
					nowSector[j] = freeMap->Find();
					printf("Extra: Indirect %d in sector %d, index %d allocate sector %d\n", oldindirect+i, indexSectors[oldindirect+i], j, nowSector[j]);
				}
//				printf("check ## sectornum = %d\n", indexSectors[i]);
				synchDisk->WriteSector(indexSectors[oldindirect+i], (char*)nowSector);
//				printf("lalala i = %d\n", i);
				delete nowSector;
			}

		}

	}
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void 
FileHeader::Deallocate(BitMap *freeMap)
{
//    for (int i = 0; i < numSectors; i++) {
//	ASSERT(freeMap->Test((int) dataSectors[i]));  // ought to be marked!
//	freeMap->Clear((int) dataSectors[i]);
//    }
	//printf("coming into Deallocate...\n");
	//freeMap->Print();
    numSectors  = divRoundUp(numBytes, SectorSize);

	if(numSectors <= NumDirect) {
		for(int i = 0; i < numSectors; i++) {
			ASSERT(freeMap->Test((int)dataSectors[i]));
			freeMap->Clear((int)dataSectors[i]);
		}
	}
	else {
		//printf("Starting Indirect index...\n");
		for(int i = 0; i < NumDirect; i++) {
			ASSERT(freeMap->Test((int)dataSectors[i]));
			freeMap->Clear((int)dataSectors[i]);
			//printf("Direct %d deallocate sector %d\n", i, dataSectors[i]);
		}
		int restSectors = numSectors - NumDirect;
		int indirect = divRoundUp(restSectors, IndirectSize/SectorSize);
		//printf("direct = %d indirect = %d indirectSectors = %d\n", NumDirect, restSectors, indirect);
		for(int i = 0; i < indirect; i++) {
			ASSERT(freeMap->Test((int)indexSectors[i]));
			int nowSectorSize = IndirectSize/SectorSize;
			if(i == indirect-1)
				nowSectorSize = restSectors - (indirect-1) * (IndirectSize/SectorSize);
			//printf("Indirect %d deallocate sector %d nowSectorSize = %d\n", i, indexSectors[i], nowSectorSize);
			int *nowSector = new int[IndirectSize/SectorSize];
//			freeMap->Print();
			synchDisk->ReadSector(indexSectors[i], (char*)nowSector);
//			for(int j = 0; j < nowSectorSize; j++) {
//				printf("^^^%d", nowSector[j]);
//			}
//			printf("\n");
//			freeMap->Print();
			for(int j = 0; j < nowSectorSize; j++) {
//				printf("nowSector[%d] = %d\n", j, nowSector[j]);
				ASSERT(freeMap->Test((int)nowSector[j]));
				freeMap->Clear((int)nowSector[j]);
				//printf("Indirect %d in sector %d, index %d deallocate sector %d\n", i, indexSectors[i], j, nowSector[j]);
			}
			freeMap->Clear((int)indexSectors[i]);
			delete nowSector;
		}
	}
	//printf("Deallocate success!\n");

}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk. 
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void
FileHeader::FetchFrom(int sector)
{
    synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk. 
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void
FileHeader::WriteBack(int sector)
{
    synchDisk->WriteSector(sector, (char *)this); 
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------

int
FileHeader::ByteToSector(int offset)
{
	int sectorNum = offset / SectorSize;
	if(sectorNum < NumDirect)
		return (dataSectors[sectorNum]);
	else {
		sectorNum = sectorNum - NumDirect + 1;
		int indexNum = sectorNum / (IndirectSize / SectorSize);
		int *nowSector = new int[IndirectSize/SectorSize];
		synchDisk->ReadSector(indexSectors[indexNum], (char*)nowSector);
		int res = nowSector[sectorNum % (IndirectSize/SectorSize) - 1];
		delete nowSector;
		return res;
	}
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int
FileHeader::FileLength()
{
    return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void
FileHeader::Print()
{
    int i, j, k;
    char *data = new char[SectorSize];

    //printf("FileHeader contents.  File size: %d.  File CreateTime: %d. File lastVisitTime: %d. File lastModifyTime: %d. File blocks:\n", numBytes, createTime, lastVisitTime, lastModifyTime);
    for (i = 0; i < numSectors; i++)
	//printf("%d ", dataSectors[i]);
    //printf("\nFile contents:\n");
    for (i = k = 0; i < numSectors; i++) {
	synchDisk->ReadSector(dataSectors[i], data);
        for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
	    if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
		printf("%c", data[j]);
            else
		printf("\\%x", (unsigned char)data[j]);
	}
        printf("\n"); 
    }
    delete [] data;
}
