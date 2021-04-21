// KERNAL Emulator
// Copyright (c) 2009-2021 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <sys/types.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "glue.h"
#include "cbmdos.h"
#include "screen.h"
#include "memory.h"
#include "time.h"
#include "ieee488.h"
#include "channelio.h"
#include "io.h"
#include "keyboard.h"
#include "vector.h"
#include "c128.h"
#include "dispatch.h"

void
kernal_init()
{
	// make sure KERNAL jump table traps
	memset(&RAM[0xFF00], 0, 0x100);

	IOINIT();
	RAMTAS();
	channelio_init();
	cbmdos_init();
}

#define NYI() printf("Unsupported KERNAL call %s at PC=$%04X S=$%02X\n", __func__, pc, sp); exit(1);

// PET BASIC commands
static void BASIC_OPEN() { NYI(); }
static void BASIC_CLOSE() { NYI(); }
static void BASIC_LOAD() { NYI(); }
static void BASIC_SAVE() { NYI(); }
static void BASIC_VERIFY() { NYI(); }
static void BASIC_SYS() { NYI(); }

// TED private
static void DEFKEY() { NYI(); }
static void PRINT() { NYI(); }
static void MONITOR() { NYI(); }
static void RESET() { NYI(); }

// C65
static void MONITOR_CALL() { NYI(); }

// The original KERNAL interface. This is the complete
// set of calls supported PET machines with BASIC 1/2/3.
// All CBM machines support these calls.
static bool
kernal_dispatch_pet()
{
	switch(pc) {
			// channel I/O
		case 0xFFC6:	CHKIN();	break;
		case 0xFFC9:	CHKOUT();	break;
		case 0xFFCC:	CLRCHN();	break;
		case 0xFFCF:	BASIN();	break;
		case 0xFFD2:	BSOUT();	break;
			// keyboard
		case 0xFFE1:	STOP();		break;
		case 0xFFE4:	GETIN();	break;
			// channel I/O
		case 0xFFE7:	CLALL();	break;
			// time
		case 0xFFEA:	UDTIM();	break;

		default:
			return false;
	}
	return true;
}

static bool
kernal_dispatch_pet_private()
{
	switch(pc) {
		case 0xF524:	OPEN();	break;
//		case 0xF563:	OPEN();	break; // CBM2

			// BASIC commands
			// These were API on the PET, but they
			// were replaced on the VIC-20.
		case 0xFFC0:	BASIC_OPEN();	break;
		case 0xFFC3:	BASIC_CLOSE();	break;
		case 0xFFD5:	BASIC_LOAD();	break;
		case 0xFFD8:	BASIC_SAVE();	break;
		case 0xFFDB:	BASIC_VERIFY();	break;
		case 0xFFDE:	BASIC_SYS();	break;

		default:
			return false;
	}
	return true;
}

// Additions for PET machines with BASIC 4
static bool
kernal_dispatch_pet4()
{
	switch(pc) {
			// IEEE-488
		case 0xFF93:	SECOND();	break;
		case 0xFF96:	TKSA();		break;
			// memory
		case 0xFF99:	MEMTOP();	break;
		case 0xFF9C:	MEMBOT();	break;
			// keyboard
		case 0xFF9F:	SCNKEY();	break;
			// IEEE-488
		case 0xFFA2:	SETTMO();	break;
		case 0xFFA5:	ACPTR();	break;
		case 0xFFA8:	CIOUT();	break;
		case 0xFFAB:	UNTLK();	break;
		case 0xFFAE:	UNLSN();	break;
		case 0xFFB1:	LISTEN();	break;
		case 0xFFB4:	TALK();		break;
			// channel I/O
		case 0xFFB7:	READST();	break;
		case 0xFFBA:	SETLFS();	break;
		case 0xFFBD:	SETNAM();	break;

		default:
			return false;
	}
	return true;
}

// Additions for the VIC-20
static bool
kernal_dispatch_vic()
{
	switch(pc) {
			// vectors
		case 0xFF8A:	RESTOR();	break;
		case 0xFF8D:	VECTOR();	break;
			// channel I/O
		case 0xFF90:	SETMSG();	break;
			// channel I/O
			// (these replace the respective PET BASIC
			// commands)
		case 0xFFC0:	OPEN();		break;
		case 0xFFC3:	CLOSE();	break;
		case 0xFFD5:	LOAD();		break;
		case 0xFFD8:	SAVE();		break;
			// time
			// (these replace the PET BASIC commands
			// VERIFY and SYS)
		case 0xFFDB:	SETTIM();	break;
		case 0xFFDE:	RDTIM();	break;
			// screen
		case 0xFFED:	SCREEN();	break;
		case 0xFFF0:	PLOT();		break;
			// VIA/CIA
		case 0xFFF3:	IOBASE();	break;

		default:
			return false;
	}
	return true;
}

// Additions for the C64. The C16/C116/+4 support
// the same set.
static bool
kernal_dispatch_c64()
{
	switch(pc) {
			// editor init
		case 0xFF81:	CINT();		break;
			// I/O init
		case 0xFF84:	IOINIT();	break;
			// RAM init
		case 0xFF87:	RAMTAS();	break;

		default:
			return false;
	}
	return true;
}

static bool
kernal_dispatch_vic20_private()
{
	switch(pc) {
		case 0xFD8D:	RAMTAS();	break;
		case 0xFDF9:	IOINIT();	break;
		case 0xE518:	/* video reset */	break;

		default:
			return false;
	}
	return true;
}


// KERNAL calls only supported on the C64. These are
// not part of the public KERNAL interface, but addresses
// that directly point to the implementations. They are
// not portable between CBM machines, but some applications
// call them anyway.
static bool
kernal_dispatch_c64_private()
{
	switch(pc) {
		case 0xE37B:	exit(0);	break;
		case 0xE386:	exit(0);	break;
		case 0xE716:	screen_bsout();	break;

		default:
			return false;
	}
	return true;
}

// Additions *only* available on the TED Series
static bool
kernal_dispatch_ted_private()
{
	switch(pc) {
			// "banking jump table"
		case 0xFCF1:    /* cartridge IRQ routine */ break;
		case 0xFCF4:    /* PHOENIX routine */ break;
		case 0xFCF7:    /* LONG FETCH routine */ break;
		case 0xFCFA:    /* LONG JUMP routine */ break;
		case 0xFCFD:    /* LONG IRQ routine */ break;

			// "unofficial jump table"
		case 0xFF49:	DEFKEY();	break;
		case 0xFF4C:	PRINT();	break;
		case 0xFF4F:	PRIMM();	break;
		case 0xFF52:	MONITOR();	break;

		case 0xFFF6:	RESET();	break;
		default:
			return false;
	}
	return true;
}

// Additions for the C128
static bool
kernal_dispatch_c128()
{
	switch(pc) {
		case 0xFF47:	SPIN_SPOUT();	break;
		case 0xFF4A:	CLOSE_ALL();	break;
		case 0xFF4D:	C64MODE();	break;
		case 0xFF50:	DMA_CALL();	break; // C65: MONITOR_CALL
		case 0xFF53:	BOOT_CALL();	break;
		case 0xFF56:	PHOENIX();	break;
		case 0xFF59:	LKUPLA();	break;
		case 0xFF5C:	LKUPSA();	break;
		case 0xFF5F:	SWAPPER();	break;
		case 0xFF62:	DLCHR();	break; // C65: missing
		case 0xFF65:	PFKEY();	break;
		case 0xFF68:	SETBNK();	break;
		case 0xFF6B:	GETCFG();	break; // C65: missing
		case 0xFF6E:	JSRFAR();	break;
		case 0xFF71:	JMPFAR();	break;
		case 0xFF74:	INDFET();	break;
		case 0xFF77:	INDSTA();	break;
		case 0xFF7A:	INDCMP();	break;
		case 0xFF7D:	PRIMM();	break;

		default:
			return false;
	}
	return true;
}

// The C65 KERNAL source is based on the C128, but has the
// following JMPs commented out:
// * DMA_CALL
// * DLCHR
// * GETCFG
// "JMP DMA_CALL" is replaced with "JMP MONITOR_CALL",
// but the other two are just removed, so the whole jump
// table is moved respective to the C128. Only calls 0xFF6E
// and above are compatible with the C128.
// Note that the preliminary docmentation says on page 40:
// "The following system calls comprise a set of extensions
// to the standard CBM jump table. They are specifically for
// the C64DX machine and and as such should not be considered
// as permanent additions to the standard jump table."
// N.B.: C65 support will need a 65CE02 emulator.
static bool
kernal_dispatch_c65_private()
{
	switch(pc) {
		case 0xFF4D:	SPIN_SPOUT();	break;
		case 0xFF50:	CLOSE_ALL();	break;
		case 0xFF53:	C64MODE();	break;
		case 0xFF56:	MONITOR_CALL();	break;
		case 0xFF59:	BOOT_CALL();	break;
		case 0xFF5C:	PHOENIX();	break;
		case 0xFF5F:	LKUPLA();	break;
		case 0xFF62:	LKUPSA();	break;
		case 0xFF65:	SWAPPER();	break;
			// missing: DLCHR
		case 0xFF68:	PFKEY();	break;
		case 0xFF6B:	SETBNK();	break;
			// missing: GETCFG
		case 0xFF6E:	JSRFAR();	break;
		case 0xFF71:	JMPFAR();	break;
		case 0xFF74:	INDFET();	break;
		case 0xFF77:	INDSTA();	break;
		case 0xFF7A:	INDCMP();	break;
		case 0xFF7D:	PRIMM();	break;

		default:
			return false;
	}
	return true;
}

bool
kernal_dispatch(machine_t machine)
{
#if 0
	if (pc != 0xffd2) {
		printf("\nkernal_dispatch $%04X; ", pc);
		printf("stack (%02X): ", sp);
		for (int i = sp + 1; i < 0x100; i++) {
			printf("%02X ", RAM[0x0100 + i]);
		}
		printf("\n");
	}
#endif

	// check KERNAL calls according to API level
	bool success = kernal_dispatch_pet();
	if (!success && machine >= MACHINE_PET4) {
		success = kernal_dispatch_pet4();
	}
	if (!success && machine >= MACHINE_VIC20) {
		success = kernal_dispatch_vic();
	}
	if (!success && machine >= MACHINE_C64) {
		success = kernal_dispatch_c64();
	}
	if (!success && machine >= MACHINE_C128) {
		success = kernal_dispatch_c128();
	}
	// TODO: CBM2
	// * supports PET4 set
	// * supports VIC-20 set, but RESTOR and VECTOR are at
	//   incompatible addresses!
	// * supports C64 CINT and IOINIT at incompatible
	//   addresses
	// * supports C128 LKUPSA and LKUPLA at incompatible
	//   addresses
	// * 6 new calls at $FF6C: TXJMP, VRESET, IPCGO, FUNKEY,
	//   IPRQST, ALOCAT; these collide with the C128 vectors

	// check private KERNAL calls on the specific machine
	if (!success && machine == MACHINE_PET) {
		success = kernal_dispatch_pet_private();
	}
	if (!success && machine == MACHINE_VIC20) {
		success = kernal_dispatch_vic20_private();
	}
	if (!success && machine == MACHINE_C64) {
		success = kernal_dispatch_c64_private();
	}
	if (!success && machine == MACHINE_TED) {
		success = kernal_dispatch_ted_private();
	}
	if (!success && machine == MACHINE_C65) {
		success = kernal_dispatch_c65_private();
	}

	// VIC-20 and above have a BRK vector
	if (!success && machine >= MACHINE_VIC20) {
		uint16_t vector = 0x0316;
		uint16_t brk = RAM[vector] | (RAM[vector + 1] << 8);

		if (brk) {
			// the CPU does this
			RAM[0x100 + sp--] = (pc + 1) >> 8;
			RAM[0x100 + sp--] = (pc + 1) & 0xff;
			RAM[0x100 + sp--] = status | 0x20;
			pc = brk;
			// KERNAL does this
			RAM[0x100 + sp--] = a;
			RAM[0x100 + sp--] = x;
			RAM[0x100 + sp--] = y;
			return true;
		}
	}

	pc = (RAM[0x100 + sp + 1] | (RAM[0x100 + sp + 2] << 8)) + 1;
	sp += 2;

	return success;
}

