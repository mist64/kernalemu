// KERNAL Emulator
// Copyright (c) 2009-2021 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <sys/types.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include "glue.h"
#include "io.h"

// IOBASE - Returns base address of I/O devices
void
IOBASE()
{
	uint16_t iobase;
	bool fake_via_random_numbers = false;
	switch (machine) {
		case MACHINE_PET:
		case MACHINE_PET4:
			// These machines don't have the call
			iobase = 0;
			break;
		case MACHINE_VIC20:
			iobase = 0x9110;
			fake_via_random_numbers = true;
			break;
		case MACHINE_C64:
			iobase = 0xdc00;
			fake_via_random_numbers = true;
			break;
		case MACHINE_TED:
			iobase = 0xfd00;
			break;
		case MACHINE_C128:
			iobase = 0xd000;
			break;
		case MACHINE_C65:
			iobase = 0xd000;
			break;
	}

	if (fake_via_random_numbers) {
		// IOBASE is used inside VIC-20/C64 BASIC's RND function
		// to get a timer value. It reads offsets 4/5 and 6/7 to
		// get the two timers of the VIA.
		// (On the C64, IOBASE points to the CIA, which has a
		// different layout, but BASIC still reads from the VIA
		// offsets, which hits at least one of the timers.)
		int pseudo_timer;
		pseudo_timer = rand();
		RAM[iobase + 4] = pseudo_timer & 0xff;
		RAM[iobase + 5] = pseudo_timer >> 8;
		pseudo_timer = rand(); // more entropy!
		RAM[iobase + 8] = pseudo_timer & 0xff;
		RAM[iobase + 9] = pseudo_timer >> 8;
		x = iobase & 0xFF;
		y = iobase >> 8;
	}
}

// IOINIT - Initialize I/O devices
void
IOINIT()
{
	srand((unsigned int)time(NULL));
}
