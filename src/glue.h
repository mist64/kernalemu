// KERNAL Emulator
// Copyright (c) 2009-2021 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdint.h>

extern uint8_t a, x, y, sp, status;
extern uint16_t pc;
extern uint8_t RAM[65536];

typedef enum {
	MACHINE_PET,
	MACHINE_PET4,
	MACHINE_VIC20,
	MACHINE_C64,
	MACHINE_TED,
	MACHINE_C128,
	MACHINE_C65,
} machine_t;

extern machine_t machine;

extern uint16_t c64_has_external_rom;

__attribute__((unused)) static void
set_c(char f)
{
	status = (status & ~1) | !!f;
}

__attribute__((unused)) static void
set_z(char f)
{
	status = (status & ~2) | (!!f << 1);
}

// This is useful to support different behavior
// based on a stack trace.
__attribute__((unused)) static int
stack4(uint16_t a, uint16_t b, uint16_t c, uint16_t d) {
#define STACK16(i) (RAM[0x0100 + i]|(RAM[0x0100 + i + 1] << 8))
	//	printf("stack4: %x,%x,%x,%x\n", a, b, c, d);
	if (STACK16(sp+1) + 1 != a) return 0;
	if (STACK16(sp+3) + 1 != b) return 0;
	if (STACK16(sp+5) + 1 != c) return 0;
	if (STACK16(sp+7) + 1 != d) return 0;
	return 1;
}
