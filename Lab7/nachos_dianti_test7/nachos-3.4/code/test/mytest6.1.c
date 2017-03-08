
#include "syscall.h"

int
main()
{
	int fileID;
	char buff[100];
	Create("lab6.txt");
	fileID = Open("lab6.txt");
	//printf("fileID = %d\n", fileID);
	Write("This is Lab6 test1!...", 22, fileID);
	fileID = Open("lab6.txt");
	Read(buff, 10, fileID);
	//printf("buff content = %s\n", buff);
	fileID = Open("lab6.txt");
	Close(fileID);
	Halt();
}
