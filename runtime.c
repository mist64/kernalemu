// Copyright (c) 2009 Michael Steil, James Abbatiello et al.
// All rights reserved. License: 2-clause BSD

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>  // getcwd, chdir
#include <windows.h> // GetLocalTime, SetLocalTime
#include <conio.h>   // _kbhit, _getch
#else
#include <sys/time.h>
#include <unistd.h>
#endif
#include "fake6502.h"
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

enum {
	MACHINE_PET,
	MACHINE_PET4,
	MACHINE_VIC20,
	MACHINE_C64,
	MACHINE_C128,
} machine;

__unused static int
stack4(uint16_t a, uint16_t b, uint16_t c, uint16_t d) {
#define STACK16(i) (RAM[0x0100+i]|(RAM[0x0100+i+1]<<8))
	//	printf("stack4: %x,%x,%x,%x\n", a, b, c, d);
	if (STACK16(sp+1) + 1 != a) return 0;
	if (STACK16(sp+3) + 1 != b) return 0;
	if (STACK16(sp+5) + 1 != c) return 0;
	if (STACK16(sp+7) + 1 != d) return 0;
	return 1;
}

static void kernal_dispatch();

uint16_t parse_num(char *s)
{
	if (s[0] == '$') {
		s++;
	} else if (s[0] == '0' && s[1] == 'x') {
		s += 2;
	}
	return strtoul(s, NULL, 16);
}
int
main(int argc, char **argv) {
	if (argc <= 1) {
		printf("Usage: %s <filenames> [<arguments>]\n", argv[0]);
		exit(1);
	}

	bool has_load_address = false;
	uint16_t load_address;
	bool has_start_address = false;
	uint16_t start_address;
	bool has_start_address_indirect = false;
	uint16_t start_address_indirect;
	bool has_machine;

	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			if (!strcmp(argv[i], "-start")) {
				if (i == argc - 1) {
					printf("%s: -start requires argument!\n", argv[0]);
					exit(1);
				}
				start_address = parse_num(argv[i + 1 ]);
				has_start_address = true;
			} else if (!strcmp(argv[i], "-startind")) {
				if (i == argc - 1) {
					printf("%s: -startind requires argument!\n", argv[0]);
					exit(1);
				}
				start_address_indirect = parse_num(argv[i + 1 ]);
				has_start_address_indirect = true;
			} else if (!strcmp(argv[i], "-machine")) {
				if (i == argc - 1) {
					printf("%s: -machine requires argument!\n", argv[0]);
					exit(1);
				}
				if (!strcmp(argv[i + 1], "pet")) {
					machine = MACHINE_PET;
				} else if (!strcmp(argv[i + 1], "pet4")) {
					machine = MACHINE_PET4;
				} else if (!strcmp(argv[i + 1], "vic20")) {
					machine = MACHINE_VIC20;
				} else if (!strcmp(argv[i + 1], "c64")) {
					machine = MACHINE_C64;
				} else if (!strcmp(argv[i + 1], "c128")) {
					machine = MACHINE_C128;
				} else {
					printf("%s: Valid values for \"-machine\" are pet, pet4, vic20, c64, c128!\n", argv[0]);
					exit(1);
				}
				has_machine = true;
			}
			i++;
		} else {
			FILE *binary = fopen(argv[i], "r");
			if (!binary) {
				printf("Error opening: %s\n", argv[i]);
				exit(1);
			}
			load_address = fgetc(binary) | fgetc(binary) << 8;
			fread(&RAM[load_address], 65536 - load_address, 1, binary);
			fclose(binary);
			has_load_address = true;
		}
	}

	if (!has_machine) {
		machine = MACHINE_C64;
	}

	//		RAM[0xfffc] = 0xd1;
	//		RAM[0xfffd] = 0xfc;
	//		RAM[0xfffe] = 0x1b;
	//		RAM[0xffff] = 0xe6;

	reset6502();
	sp = 0xff;

	if (has_start_address) {
		pc = start_address;
	} else if (has_start_address_indirect) {
		pc = RAM[start_address_indirect] | (RAM[start_address_indirect + 1] << 8);
	} else if (has_load_address) {
		pc = load_address;
	} else {
		printf("%s: You need to specify at least one binary file!\n", argv[0]);
		exit(1);
	}

	IOINIT();
	RAMTAS();

	cbmdos_init();

	for (;;) {
//		printf("pc = %04x; %02x %02x %02x\n", pc, RAM[pc], RAM[pc+1], RAM[pc+2]);
		step6502();
		if (!RAM[pc]) {
			kernal_dispatch();
		}
	}

	return 0;
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

static void
kernal_dispatch()
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

