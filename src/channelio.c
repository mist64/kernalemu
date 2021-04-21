// KERNAL Emulator
// Copyright (c) 2009-2021 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include "glue.h"
#include "error.h"
#include "cbmdos.h"
#include "screen.h"
#include "printer.h"
#include "channelio.h"

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

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

uint8_t kernal_msgflag;
uint8_t *STATUS_p;
uint16_t FNADR;
uint8_t FNLEN;
uint8_t LA, FA, SA;
uint8_t DFLTO = KERN_DEVICE_SCREEN;
uint8_t DFLTN = KERN_DEVICE_KEYBOARD;

#define STATUS (*STATUS_p)

uint8_t file_to_device[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

void
channelio_init()
{
	STATUS_p = &RAM[0x90];
	STATUS = 0;
}

// SETMSG - Control KERNAL messages
void
SETMSG()
{
	// TODO: Act on kernal_msgflag
	kernal_msgflag = a;
	a = STATUS;
}

// READST - Read I/O status word
void
READST()
{
	a = STATUS;
}

// SETLFS - Set logical, first, and second addresses
void
SETLFS()
{
	LA = a;
	FA = x;
	SA = y;
}

// SETNAM - Set file name
void
SETNAM()
{
	FNADR = x | y << 8;
	FNLEN = a;
}

// OPEN - Open a logical file
void
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
		case KERN_DEVICE_SCREEN:
		case KERN_DEVICE_KEYBOARD:
			a = KERN_ERR_NONE;
			break;
		case KERN_DEVICE_CASSETTE:
		case KERN_DEVICE_RS232:
			a = KERN_ERR_DEVICE_NOT_PRESENT;
			break;
		case KERN_DEVICE_PRINTERU4:
			printer_open(4);
			a = KERN_ERR_NONE;
			break;
		case KERN_DEVICE_PRINTERU5:
			printer_open(5);
			a = KERN_ERR_NONE;
			break;
		case KERN_DEVICE_PRINTERU6:
			printer_open(6);
			a = KERN_ERR_NONE;
			break;
		case KERN_DEVICE_PRINTERU7:
			printer_open(7);
			a = KERN_ERR_NONE;
			break;
		case KERN_DEVICE_DRIVEU8:
		case KERN_DEVICE_DRIVEU9:
		case KERN_DEVICE_DRIVEU10:
		case KERN_DEVICE_DRIVEU11:
		case KERN_DEVICE_DRIVEU12:
		case KERN_DEVICE_DRIVEU13:
		case KERN_DEVICE_DRIVEU14:
		case KERN_DEVICE_DRIVEU15:
			a = cbmdos_open(LA, FA, SA, filename);
			break;
	}

	if (a == KERN_ERR_NONE) {
		file_to_device[LA] = FA;
	}
//	printf("file_to_device[%d] = %d\n", LA, FA);
	set_c(a != KERN_ERR_NONE);
}

// CLOSE - Close a specified logical file
void
CLOSE()
{
	uint8_t dev = file_to_device[a];
//	printf("CLOSE %d,%d\n", a, dev);
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
			set_c(0);
			return;
		case KERN_DEVICE_PRINTERU4:
			printer_close(4);
			a = KERN_ERR_NONE;
			break;
		case KERN_DEVICE_PRINTERU5:
			printer_close(5);
			a = KERN_ERR_NONE;
			break;
		case KERN_DEVICE_PRINTERU6:
			printer_close(6);
			a = KERN_ERR_NONE;
			break;
		case KERN_DEVICE_PRINTERU7:
			printer_close(7);
			a = KERN_ERR_NONE;
			break;
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

// CHKIN - Open channel for input
void
CHKIN()
{
	uint8_t dev = file_to_device[x];
//	printf("CHKIN %d (dev %d)\n", x, dev);
	if (dev == 0xFF) {
		DFLTN = KERN_DEVICE_KEYBOARD;
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

// CHKOUT - Open channel for output
void
CHKOUT()
{
	uint8_t dev = file_to_device[x];
//	printf("CHKOUT %d (dev %d)\n", x, dev);
	if (dev == 0xFF) {
		DFLTO = KERN_DEVICE_SCREEN;
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

// CLRCHN - Close input and output channels
void
CLRCHN()
{
	DFLTO = KERN_DEVICE_SCREEN;
	DFLTN = KERN_DEVICE_KEYBOARD;
}

// BASIN - Input character from channel
void
BASIN()
{
//		printf("%s:%d DFLTN: %d\n", __func__, __LINE__, DFLTN);
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


// BSOUT - Output character to channel
void
BSOUT()
{
#if 0
	int a1 = *(uint16_t *)(&RAM[0x0100+sp+1]) + 1;
	int a2 = *(uint16_t *)(&RAM[0x0100+sp+3]) + 1;
	int a3 = *(uint16_t *)(&RAM[0x0100+sp+5]) + 1;
	int a4 = *(uint16_t *)(&RAM[0x0100+sp+7]) + 1;
	printf("+BSOUT: '%c' -> %d @ %x,%x,%x,%x: ---", a, DFLTO, a1, a2, a3, a4);
#endif
//	printf("<%s:%d DFLTO: %d>", __func__, __LINE__, DFLTO);
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
			printer_bsout(4);
			set_c(0);
			return;
		case KERN_DEVICE_PRINTERU5:
			printer_bsout(5);
			set_c(0);
			return;
		case KERN_DEVICE_PRINTERU6:
			printer_bsout(6);
			set_c(0);
			return;
		case KERN_DEVICE_PRINTERU7:
			printer_bsout(7);
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
			STATUS = cbmdos_bsout(DFLTO, a);
			set_c(STATUS);
			break;
	}
	//	printf("--- BSOUT: '%c' -> %d @ %x,%x,%x,%x\n", a, DFLTO, a1, a2, a3, a4);
//	printf("</>");
}

// LOAD - Load RAM from a device
void
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

// SAVE - Save RAM to device
void
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

// CLALL - Close all channels and files
void
CLALL()
{
//	printf("CLALL\n");
	for (a = 0; a < sizeof(file_to_device)/sizeof(file_to_device[0]); a++) {
		uint8_t dev = file_to_device[a];
		if (dev != 0xFF) {
			CLOSE();
		}
	}
}
