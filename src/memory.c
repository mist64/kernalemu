// KERNAL Emulator
// Copyright (c) 2009-2021 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <sys/types.h>
#include <string.h>
#include "glue.h"
#include "memory.h"

uint16_t ram_bot, ram_top;

//
// interface for fake6502
//

uint8_t RAM[65536];

uint8_t
read6502(uint16_t address)
{
	return RAM[address];
}

void
write6502(uint16_t address, uint8_t value)
{
	RAM[address] = value;
}

// RAMTAS - Perform RAM test
void
RAMTAS()
{
	switch (machine) {
		case MACHINE_PET:
		case MACHINE_PET4:
			// impossible
			break;
		case MACHINE_VIC20:
			ram_bot = 0x1200; // unexpanded: 0x1000
			ram_top = 0x8000; // unexpanded: 0x1e00
			break;
		case MACHINE_C64:
			ram_bot = 0x0800;
			ram_top = c64_has_external_rom ? 0x8000 : 0xa000;
			break;
		case MACHINE_TED:
			ram_bot = 0x1000;
			ram_top = 0xfd00;
			break;
		case MACHINE_C128:
			ram_bot = 0x1c00;
			ram_top = 0xff00;
			break;
		case MACHINE_C65:
			ram_bot = 0x2000;
			ram_top = 0xff00;
			break;
	}

	// clear zero page
	memset(RAM, 0, 0x100);
	// clear 0x200-0x3ff
	memset(&RAM[0x0200], 0, 0x200);
}

// MEMTOP - Read/set the top of memory
void
MEMTOP()
{
	if (status & 1) {
		x = ram_top & 0xFF;
		y = ram_top >> 8;
	} else {
		ram_top = x | (y << 8);
	}
}

// MEMBOT - Read/set the bottom of memory
void
MEMBOT()
{
	if (status & 1) {
		x = ram_bot & 0xFF;
		y = ram_bot >> 8;
	} else {
		ram_bot = x | (y << 8);
	}
}
