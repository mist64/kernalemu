#include <sys/types.h>
#include <stdlib.h>
#include <time.h>
#include "glue.h"
#include "io.h"

// IOBASE - Returns base address of I/O devices
void
IOBASE()
{
#define CIA 0xDC00 // we could put this anywhere...
	// IOBASE is just used inside RND to get a timer value.
	// So, let's fake this here, too.
	// Commodore BASIC reads offsets 4/5 and 6/7 to get the
	// two timers of the CIA.
	int pseudo_timer;
	pseudo_timer = rand();
	RAM[CIA+4] = pseudo_timer&0xff;
	RAM[CIA+5] = pseudo_timer>>8;
	pseudo_timer = rand(); // more entropy!
	RAM[CIA+8] = pseudo_timer&0xff;
	RAM[CIA+9] = pseudo_timer>>8;
	x = CIA & 0xFF;
	y = CIA >> 8;
}

// IOINIT - Initialize I/O devices
void
IOINIT()
{
	srand((unsigned int)time(NULL));
}
