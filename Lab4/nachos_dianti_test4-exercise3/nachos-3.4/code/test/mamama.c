/* matmult.c 
 *    Test program to do matrix multiplication on large arrays.
 *
 *    Intended to stress virtual memory system.
 *
 *    Ideally, we could read the matrices off of the file system,
 *	and store the result back to the file system!
 */

#include "syscall.h"

#define Dim 	1	/* sum total of the arrays doesn't fit in 
			 * physical memory 
			 */

int
main()
{
    int i, j, k;
	int p, q, r;
	int numnum;
	i = 1;
	j = 2;
	k = i + j;
	p = 4;
	q = 5;
	r = 6;
	p = q*r;
	k = q*i;
	j = p*r;
	numnum = k + i + j + p + q + r;
	Halt();
}
