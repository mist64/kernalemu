// Copyright (c) 2009 Michael Steil, James Abbatiello et al.
// All rights reserved. License: 2-clause BSD

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <direct.h>  // getcwd, chdir
#include <windows.h> // GetLocalTime, SetLocalTime
#include <conio.h>   // _kbhit, _getch
#else
#include <sys/time.h>
#include <unistd.h>
#endif
#include "fake6502.h"
#include "stat.h"
#include "readdir.h"
#include "glue.h"
#include "console.h"
#include "error.h"
#include "cbmdos.h"
#include "screen.h"

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

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

void
set_c(char f)
{
	status = (status & ~1) | !!f;
}

void
set_z(char f)
{
	status = (status & ~2) | (!!f << 1);
}

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

//***********************************************************
// KERNAL interface implementation                          *
// http://members.tripod.com/~Frank_Kontros/kernal/addr.htm *
//***********************************************************

// KERNAL constants
#if 0
#define RAM_BOT 0x0400 // we could just as well start at 0x0400, as there is no screen RAM 
#else
#define RAM_BOT 0x0800
#endif
#define RAM_TOP 0xA000

#define KERN_DEVICE_KEYBOARD  0
#define KERN_DEVICE_CASSETTE  1
#define KERN_DEVICE_RS232     2
#define KERN_DEVICE_SCREEN    3
#define KERN_DEVICE_PRINTERU4 4
#define KERN_DEVICE_PRINTERU5 5
#define KERN_DEVICE_PRINTERU6 6
#define KERN_DEVICE_PRINTERU7 7
#define KERN_DEVICE_DRIVEU8   8
#define KERN_DEVICE_DRIVEU9   9
#define KERN_DEVICE_DRIVEU10  10
#define KERN_DEVICE_DRIVEU11  11
#define KERN_DEVICE_DRIVEU12  12
#define KERN_DEVICE_DRIVEU13  13
#define KERN_DEVICE_DRIVEU14  14
#define KERN_DEVICE_DRIVEU15  15

// KERNAL internal state
uint8_t kernal_msgflag, STATUS = 0;
uint16_t FNADR;
uint8_t FNLEN;
uint8_t LA, FA, SA;
uint8_t DFLTO = KERN_DEVICE_SCREEN;
uint8_t DFLTN = KERN_DEVICE_KEYBOARD;

uint8_t file_to_device[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

// shell script hack
int readycount = 0;

int kernal_dispatch();

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

	srand((unsigned int)time(NULL));

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

static void
SETMSG()
{
	kernal_msgflag = a;
	a = STATUS;
}

static void
MEMTOP()
{
#if DEBUG // CBMBASIC doesn't do this
	if (!C) {
		printf("UNIMPL: set top of RAM");
		exit(1);
	}
#endif
	x = RAM_TOP&0xFF;
	y = RAM_TOP>>8;
}

// MEMBOT
static void
MEMBOT()
{
#if DEBUG // CBMBASIC doesn't do this
	if (!C) {
		printf("UNIMPL: set bot of RAM");
		exit(1);
	}
#endif
	x = RAM_BOT&0xFF;
	y = RAM_BOT>>8;
}

// READST
static void
READST()
{
	a = STATUS;
}

// SETLFS
static void
SETLFS()
{
	LA = a;
	FA = x;
	SA = y;
}

// SETNAM
static void
SETNAM()
{
	FNADR = x | y << 8;
	FNLEN = a;
}

// OPEN
static void
OPEN()
{
	STATUS = 0;
	if (file_to_device[LA] != 0xFF) {
		set_c(1);
		a = KERN_ERR_FILE_OPEN;
		return;
	}

	char filename[41];
	uint8_t len = MIN(FNLEN, sizeof(filename) - 1);
	memcpy(filename, (char *)&RAM[FNADR], FNLEN);
	filename[len] = 0;
//	printf("OPEN %d,%d,%d,\"%s\"\n", LA, FA, SA, filename);

	switch (FA) {
		case KERN_DEVICE_KEYBOARD:
		case KERN_DEVICE_CASSETTE:
		case KERN_DEVICE_RS232:
			set_c(1);
			a = KERN_ERR_DEVICE_NOT_PRESENT;
			return;
		case KERN_DEVICE_SCREEN:
			break;
		case KERN_DEVICE_PRINTERU4:
		case KERN_DEVICE_PRINTERU5:
		case KERN_DEVICE_PRINTERU6:
		case KERN_DEVICE_PRINTERU7:
			set_c(1);
			a = KERN_ERR_DEVICE_NOT_PRESENT;
			return;
		case KERN_DEVICE_DRIVEU8:
		case KERN_DEVICE_DRIVEU9:
		case KERN_DEVICE_DRIVEU10:
		case KERN_DEVICE_DRIVEU11:
		case KERN_DEVICE_DRIVEU12:
		case KERN_DEVICE_DRIVEU13:
		case KERN_DEVICE_DRIVEU14:
		case KERN_DEVICE_DRIVEU15:
			a = cbmdos_open(LA, FA, SA, filename);
			set_c(a != KERN_ERR_NONE);
			break;
	}

	if (!(a & 1)) {
		file_to_device[LA] = FA;
	}
//	printf("file_to_device[%d] = %d\n", LA, FA);
}

// CLOSE
static void
CLOSE()
{
	uint8_t dev = file_to_device[a];
	if (dev == 0xFF) {
		set_c(1);
		a = KERN_ERR_FILE_NOT_OPEN;
		return;
	}

	switch (dev) {
		case KERN_DEVICE_KEYBOARD:
		case KERN_DEVICE_CASSETTE:
		case KERN_DEVICE_RS232:
		case KERN_DEVICE_SCREEN:
		case KERN_DEVICE_PRINTERU4:
		case KERN_DEVICE_PRINTERU5:
		case KERN_DEVICE_PRINTERU6:
		case KERN_DEVICE_PRINTERU7:
			set_c(0);
			return;
		case KERN_DEVICE_DRIVEU8:
		case KERN_DEVICE_DRIVEU9:
		case KERN_DEVICE_DRIVEU10:
		case KERN_DEVICE_DRIVEU11:
		case KERN_DEVICE_DRIVEU12:
		case KERN_DEVICE_DRIVEU13:
		case KERN_DEVICE_DRIVEU14:
		case KERN_DEVICE_DRIVEU15:
			cbmdos_close(a, dev);
			set_c(0);
			break;
	}

	file_to_device[a] = 0xFF;
}

// CHKIN
static void
CHKIN()
{
	uint8_t dev = file_to_device[x];
//	printf("CHKIN %d (dev %d)\n", x, dev);
	if (dev == 0xFF) {
		set_c(1);
		a = KERN_ERR_FILE_NOT_OPEN;
		return;
	}

	switch (dev) {
		case KERN_DEVICE_KEYBOARD:
		case KERN_DEVICE_CASSETTE:
		case KERN_DEVICE_RS232:
		case KERN_DEVICE_SCREEN:
		case KERN_DEVICE_PRINTERU4:
		case KERN_DEVICE_PRINTERU5:
		case KERN_DEVICE_PRINTERU6:
		case KERN_DEVICE_PRINTERU7:
			set_c(0);
			break;
		case KERN_DEVICE_DRIVEU8:
		case KERN_DEVICE_DRIVEU9:
		case KERN_DEVICE_DRIVEU10:
		case KERN_DEVICE_DRIVEU11:
		case KERN_DEVICE_DRIVEU12:
		case KERN_DEVICE_DRIVEU13:
		case KERN_DEVICE_DRIVEU14:
		case KERN_DEVICE_DRIVEU15:
			cbmdos_chkin(x, dev);
			set_c(0);
			break;
	}
	DFLTN = dev;
}

// CHKOUT
static void
CHKOUT()
{
//	printf("CHKOUT %d\n", x);
	uint8_t dev = file_to_device[x];
	if (dev == 0xFF) {
		set_c(1);
		a = KERN_ERR_FILE_NOT_OPEN;
		return;
	}

	switch (dev) {
		case KERN_DEVICE_KEYBOARD:
		case KERN_DEVICE_CASSETTE:
		case KERN_DEVICE_RS232:
		case KERN_DEVICE_SCREEN:
		case KERN_DEVICE_PRINTERU4:
		case KERN_DEVICE_PRINTERU5:
		case KERN_DEVICE_PRINTERU6:
		case KERN_DEVICE_PRINTERU7:
			set_c(0);
			break;
		case KERN_DEVICE_DRIVEU8:
		case KERN_DEVICE_DRIVEU9:
		case KERN_DEVICE_DRIVEU10:
		case KERN_DEVICE_DRIVEU11:
		case KERN_DEVICE_DRIVEU12:
		case KERN_DEVICE_DRIVEU13:
		case KERN_DEVICE_DRIVEU14:
		case KERN_DEVICE_DRIVEU15:
			cbmdos_chkout(x, dev);
			set_c(0);
			break;
	}

	DFLTO = dev;
//	printf("%s:%d DFLTO: %d\n", __func__, __LINE__, DFLTO);
}

// CLRCHN
static void
CLRCHN()
{
	DFLTO = KERN_DEVICE_SCREEN;
	DFLTN = KERN_DEVICE_KEYBOARD;
}

// BASIN
static void
BASIN()
{
//	printf("%s:%d DFLTN: %d\n", __func__, __LINE__, DFLTN);
	switch (DFLTN) {
		case KERN_DEVICE_KEYBOARD:
			a = getchar(); // stdin
			if (a == '\n') {
				a = '\r';
			}
			break;
		case KERN_DEVICE_CASSETTE:
		case KERN_DEVICE_RS232:
		case KERN_DEVICE_SCREEN:
		case KERN_DEVICE_PRINTERU4:
		case KERN_DEVICE_PRINTERU5:
		case KERN_DEVICE_PRINTERU6:
		case KERN_DEVICE_PRINTERU7:
			set_c(1);
			return;
		case KERN_DEVICE_DRIVEU8:
		case KERN_DEVICE_DRIVEU9:
		case KERN_DEVICE_DRIVEU10:
		case KERN_DEVICE_DRIVEU11:
		case KERN_DEVICE_DRIVEU12:
		case KERN_DEVICE_DRIVEU13:
		case KERN_DEVICE_DRIVEU14:
		case KERN_DEVICE_DRIVEU15:
			a = cbmdos_basin(DFLTN, &STATUS);
			set_c(0);
			break;
	}
}


// BSOUT
static void
BSOUT()
{
#if 0
	int a1 = *(uint16_t *)(&RAM[0x0100+sp+1]) + 1;
	int a2 = *(uint16_t *)(&RAM[0x0100+sp+3]) + 1;
	int a3 = *(uint16_t *)(&RAM[0x0100+sp+5]) + 1;
	int a4 = *(uint16_t *)(&RAM[0x0100+sp+7]) + 1;
	printf("+BSOUT: '%c' -> %d @ %x,%x,%x,%x: ---", a, DFLTO, a1, a2, a3, a4);
#endif
//	printf("%s:%d DFLTO: %d\n", __func__, __LINE__, DFLTO);
	switch (DFLTO) {
		case KERN_DEVICE_KEYBOARD:
			set_c(1);
			return;
		case KERN_DEVICE_CASSETTE:
			set_c(1);
			return;
		case KERN_DEVICE_RS232:
			set_c(1);
			return;
		case KERN_DEVICE_SCREEN:
			screen_bsout();
			set_c(0);
			break;
		case KERN_DEVICE_PRINTERU4:
		case KERN_DEVICE_PRINTERU5:
		case KERN_DEVICE_PRINTERU6:
		case KERN_DEVICE_PRINTERU7:
			set_c(1);
			return;
		case KERN_DEVICE_DRIVEU8:
		case KERN_DEVICE_DRIVEU9:
		case KERN_DEVICE_DRIVEU10:
		case KERN_DEVICE_DRIVEU11:
		case KERN_DEVICE_DRIVEU12:
		case KERN_DEVICE_DRIVEU13:
		case KERN_DEVICE_DRIVEU14:
		case KERN_DEVICE_DRIVEU15:
			STATUS = cbmdos_bsout(DFLTO, a);
			set_c(STATUS);
			break;
	}
	//	printf("--- BSOUT: '%c' -> %d @ %x,%x,%x,%x\n", a, DFLTO, a1, a2, a3, a4);
}

// LOAD
static void
LOAD()
{
	int error = KERN_ERR_NONE;

	uint16_t override_address = (x | (y << 8));

	SA = 0; // read
	OPEN();
	if (status & 1) {
		a = status;
		return;
	}

	x = LA;
	CHKIN();

	BASIN();
	if (STATUS & KERN_ST_EOF) {
		error = KERN_ERR_FILE_NOT_FOUND;
		goto end;
	}
	uint16_t address = a;
	BASIN();
	if (STATUS & KERN_ST_EOF) {
		error = KERN_ERR_FILE_NOT_FOUND;
		goto end;
	}
	address |= a << 8;
	if (!SA) {
		address = override_address;
	}
//	printf("%04x\n", address);

	do {
		BASIN();
		RAM[address++] = a;
	} while (!(STATUS & KERN_ST_EOF));

end:
	CLRCHN();
	a = LA;
	CLOSE();
	if (error != KERN_ERR_NONE) {
		set_c(1);
		a = error;
	} else {
		x = address & 0xff;
		y = address >> 8;
		set_c(0);
		STATUS = 0;
		a = 0;
	}
//	for (int i = 0; i < 255; i++) {
//	 if (!(i & 15)) {
//		 printf("\n %04X  ", 0x0800+i);
//	 }
//	 printf("%02X ", RAM[0x0800+i]);
//	}
}

// SAVE
static void
SAVE()
{
	uint16_t address = RAM[a] | RAM[a + 1] << 8;
	uint16_t end = x | y << 8;
	if (end < address) {
		set_c(1);
		a = KERN_ERR_NONE;
		return;
	}

	int error = KERN_ERR_NONE;

	SA = 1; // write
	OPEN();
	if (status & 1) {
		a = status;
		return;
	}

	x = LA;
	CHKOUT();

	a = address & 0xff;;
	BSOUT();
	a = address >> 8;
	BSOUT();

	while (address != end) {
		a = RAM[address++];
		BSOUT();
	};

	CLRCHN();
	a = LA;
	CLOSE();
	if (error != KERN_ERR_NONE) {
		set_c(1);
		a = error;
	} else {
		set_c(0);
		STATUS = 0;
		a = 0;
	}
}

// SETTIM
static void
SETTIM()
{
	unsigned long   jiffies = y*65536 + x*256 + a;
	unsigned long   seconds = jiffies/60;
#ifdef _WIN32
	SYSTEMTIME st;

	GetLocalTime(&st);
	st.wHour         = (WORD)(seconds/3600);
	st.wMinute       = (WORD)(seconds/60);
	st.wSecond       = (WORD)(seconds%60);
	st.wMilliseconds = (WORD)((jiffies % 60) * 1000 / 60);
	SetLocalTime(&st);
#else
	time_t now = time(0);
	struct tm bd;
	struct timeval tv;

	localtime_r(&now, &bd);

	bd.tm_sec = seconds % 60;
	bd.tm_min = seconds / 60;
	bd.tm_hour = seconds / 3600;

	tv.tv_sec = mktime(&bd);
	tv.tv_usec = (jiffies % 60) * (1000000 / 60);

	settimeofday(&tv, 0);
#endif
}

// RDTIM
static void
RDTIM()
{
	unsigned long   jiffies;
#ifdef _WIN32
	SYSTEMTIME st;

	GetLocalTime(&st);
	jiffies = ((st.wHour*60 + st.wMinute)*60 + st.wSecond)*60 + st.wMilliseconds * 60 / 1000;
#else
	time_t now = time(0);
	struct tm bd;
	struct timeval tv;

	localtime_r(&now, &bd);
	gettimeofday(&tv, 0);

	jiffies = ((bd.tm_hour * 60 + bd.tm_min) * 60 + bd.tm_sec) * 60 + tv.tv_usec / (1000000 / 60);
#endif
	y   = (uint8_t)(jiffies / 65536);
	x   = (uint8_t)((jiffies % 65536) / 256);
	a   = (uint8_t)(jiffies % 256);

}

// STOP
static void
STOP()
{
	set_z(0); // TODO we don't support the STOP key
}

// GETIN
static void
GETIN()
{
	BASIN();
}

// CLALL
static void
CLALL()
{
	for (a = 0; a < sizeof(file_to_device)/sizeof(file_to_device[0]); a++) {
		uint8_t dev = file_to_device[a];
		if (dev != 0xFF) {
			CLOSE();
		}
	}
}

// PLOT
static void
PLOT()
{
	if (status & 1) {
		int CX, CY;
		get_cursor(&CX, &CY);
		y = CX;
		x = CY;
	} else {
		printf("UNIMPL: set cursor %d %d\n", y, x);
		exit(1);
	}
}


// IOBASE
static void
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

int
kernal_dispatch()
{
#if 0
	if (pc != 0xffd2) {
		printf("kernal_dispatch $%04X; ", pc);
		printf("stack (%02X): ", sp);
		for (int i=sp+1; i<0x100; i++) {
			printf("%02X ", RAM[0x0100+i]);
		}
		printf("\n");
	}
#endif

	switch(pc) {
		case 0xE386:	exit(0);	break;
		case 0xE716:	/*screen_bsout();*/	break;
		case 0xFF90:	SETMSG();	break;
		case 0xFF99:	MEMTOP();	break;
		case 0xFF9C:	MEMBOT();	break;
		case 0xFFB7:	READST();	break;
		case 0xFFBA:	SETLFS();	break;
		case 0xFFBD:	SETNAM();	break;
		case 0xFFC0:	OPEN();		break;
		case 0xFFC3:	CLOSE();	break;
		case 0xFFC6:	CHKIN();	break;
		case 0xFFC9:	CHKOUT();	break;
		case 0xFFCC:	CLRCHN();	break;
		case 0xFFCF:	BASIN();	break;
		case 0xFFD2:	BSOUT();	break;
		case 0xFFD5:	LOAD();		break;
		case 0xFFD8:	SAVE();		break;
		case 0xFFDB:	SETTIM();	break;
		case 0xFFDE:	RDTIM();	break;
		case 0xFFE1:	STOP();		break;
		case 0xFFE4:	GETIN();	break;
		case 0xFFE7:	CLALL();	break;
		case 0xFFF0:	PLOT();		break;
		case 0xFFF3:	IOBASE();	break;

		default: printf("unknown PC=$%04X S=$%02X\n", pc, sp); exit(1);
	}
	pc = (RAM[0x100+sp+1] | (RAM[0x100+sp+2] << 8)) + 1;
	sp += 2;
	return 1;
}

