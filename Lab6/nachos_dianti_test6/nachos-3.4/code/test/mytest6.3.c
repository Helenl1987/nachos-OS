
#include "syscall.h"

void func() {
	Exit(0);
}

int
main()
{
	Fork(func);
	Yield();
	Exit(0);
}
