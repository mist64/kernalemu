#include <sys/types.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
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
	IOINIT();
	RAMTAS();
	cbmdos_init();
}

static bool
kernal_dispatch_pet()
{
	switch(pc) {
		case 0xFFC0:	OPEN();		break;
		case 0xFFC3:	CLOSE();	break;
		case 0xFFC6:	CHKIN();	break;
		case 0xFFC9:	CHKOUT();	break;
		case 0xFFCC:	CLRCHN();	break;
		case 0xFFCF:	BASIN();	break;
		case 0xFFD2:	BSOUT();	break;
		case 0xFFD5:	LOAD();		break;
		case 0xFFD8:	SAVE();		break;
			// time
		case 0xFFDB:	SETTIM();	break;
		case 0xFFDE:	RDTIM();	break;
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
kernal_dispatch_pet4()
{
	switch(pc) {
			// IEC
		case 0xFF93:	SECOND();	break;
		case 0xFF96:	TKSA();		break;
			// memory
		case 0xFF99:	MEMTOP();	break;
		case 0xFF9C:	MEMBOT();	break;
			// keyboard
		case 0xFF9F:	SCNKEY();	break;
			// IEC
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

static bool
kernal_dispatch_vic()
{
	switch(pc) {
			// vectors
		case 0xFF8A:	RESTOR();	break;
		case 0xFF8D:	VECTOR();	break;
			// channel I/O
		case 0xFF90:	SETMSG();	break;
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
kernal_dispatch_c128()
{
	switch(pc) {
		case 0xFF47:	SPIN_SPOUT();	break;
		case 0xFF4A:	CLOSE_ALL();	break;
		case 0xFF4D:	C64MODE();	break;
		case 0xFF50:	DMA_CALL();	break;
		case 0xFF53:	BOOT_CALL();	break;
		case 0xFF56:	PHOENIX();	break;
		case 0xFF59:	LKUPLA();	break;
		case 0xFF5C:	LKUPSA();	break;
		case 0xFF5F:	SWAPPER();	break;
		case 0xFF62:	DLCHR();	break;
		case 0xFF65:	PFKEY();	break;
		case 0xFF68:	SETBNK();	break;
		case 0xFF6B:	GETCFG();	break;
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

static bool
kernal_dispatch_c64_internal()
{
	switch(pc) {
		case 0xE386:	exit(0);	break;
		case 0xE716:	/*screen_bsout();*/	break;

		default:
			return false;
	}
	return true;
}

void
kernal_dispatch(machine_t machine)
{
#if 0
	if (pc != 0xffd2) {
		printf("kernal_dispatch $%04X; ", pc);
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
	// CBM2:
	// * has vectors $FF90-$FFF3 (so it's compatible with PET 1/2/3/4)
	// * extra vectors $FF6C-$FF8D are incompatible

	// check private KERNAL calls on the specicif machine
	if (!success && machine == MACHINE_C64) {
		success = kernal_dispatch_c64_internal();
	}

	if (!success) {
		printf("unknown PC=$%04X S=$%02X\n", pc, sp);
		exit(1);
	}

	pc = (RAM[0x100 + sp + 1] | (RAM[0x100 + sp + 2] << 8)) + 1;
	sp += 2;
}

