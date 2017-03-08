
#include "syscall.h"

int
main()
{
	int id;
	//Open("../test/halt");
	id = Exec("../test/halt");
	Join(id);
	Exit(0);
}
