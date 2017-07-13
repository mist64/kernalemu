/*
 * Copyright (c) 2009 Michael Steil, James Abbatiello
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

//#define NO_CLRHOME

#include <stdlib.h>
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
#include "stat.h"
#include "readdir.h"
#include "glue.h"
#include "console.h"

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

unsigned char RAM[65536];

//unsigned char A, X, Y, S;
//unsigned short PC;
//unsigned char V, B, D, I, C, N, Z;

extern void reset6502();
void step6502();
extern void exec6502(uint32_t tickcount);

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

uint8_t
read6502(uint16_t address)
{
	if (address >= 0xa000) {
//		printf("R %04x\n", address);
	}
	return RAM[address];
}

void
write6502(uint16_t address, uint8_t value)
{
	if (address >= 0xa000) {
//		printf("W %04x\n", address);
	}
	RAM[address] = value;
}

int
stack4(unsigned short a, unsigned short b, unsigned short c, unsigned short d) {
//	printf("stack4: %x,%x,%x,%x\n", a, b, c, d);
	if (STACK16(sp+1) + 1 != a) return 0;
	if (STACK16(sp+3) + 1 != b) return 0;
	if (STACK16(sp+5) + 1 != c) return 0;
	if (STACK16(sp+7) + 1 != d) return 0;
	return 1;
}

/************************************************************/
/* KERNAL interface implementation                          */
/* http://members.tripod.com/~Frank_Kontros/kernal/addr.htm */
/************************************************************/

/* KERNAL constants */
#if 0
#define RAM_BOT 0x0400 /* we could just as well start at 0x0400, as there is no screen RAM */
#else
#define RAM_BOT 0x0800
#endif
#define RAM_TOP 0xA000
#define KERN_ERR_NONE			0
#define KERN_ERR_FILE_OPEN			2
#define KERN_ERR_FILE_NOT_OPEN		3
#define KERN_ERR_FILE_NOT_FOUND		4
#define KERN_ERR_DEVICE_NOT_PRESENT	5
#define KERN_ERR_NOT_INPUT_FILE		6
#define KERN_ERR_NOT_OUTPUT_FILE	7
#define KERN_ERR_MISSING_FILE_NAME	8
#define KERN_ERR_ILLEGAL_DEVICE_NUMBER	9

#define KERN_ST_TIME_OUT_READ 0x02
#define KERN_ST_EOF 0x40

/* KERNAL internal state */
unsigned char kernal_msgflag, kernal_status = 0;
unsigned short kernal_filename;
unsigned char kernal_filename_len;
unsigned char kernal_lfn, kernal_dev, kernal_sec;
int kernal_quote = 0;
unsigned char kernal_output = 0, kernal_input = 0;
FILE* kernal_files[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
int kernal_files_next[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

/* shell script hack */
int readycount = 0;

int kernal_dispatch();

int
main(int argc, char **argv) {
	if (argc>1) {
		FILE *binary = fopen(argv[1], "r");
		if (!binary) {
			printf("Error opening: %s\n", argv[1]);
			exit(1);
		}
		uint16_t load_address = fgetc(binary);
		load_address |= fgetc(binary) << 8;
		fread(&RAM[load_address], 65536 - load_address, 1, binary);
		fclose(binary);

		RAM[0xfffc] = 0xd1;
		RAM[0xfffd] = 0xfc;
		RAM[0xfffe] = 0x1b;
		RAM[0xffff] = 0xe6;
	} else {
		printf("Usage: %s <filename>\n", argv[0]);
		exit(1);
	}
	srand((unsigned int)time(NULL));

	reset6502();
	pc = 2063; /* entry point of assembler64 */

	for (;;) {
		step6502();
//		printf("pc = %04x; %02x %02x %02x\n", pc, RAM[pc], RAM[pc+1], RAM[pc+2]);
		if (!RAM[pc]) {
			kernal_dispatch();
		}
	}

	return 0;
}

unsigned short orig_error, orig_main, orig_crnch, orig_qplop, orig_gone, orig_eval;

void
replace_vector(unsigned short address, unsigned short new, unsigned short *old) {
	*old = RAM[address] | (RAM[address+1]<<8);
	RAM[address] = (new)&0xFF;
	RAM[address+1] = (new)>>8;
}

static void
SETMSG() {
		kernal_msgflag = a;
		a = kernal_status;
}

static void
MEMTOP() {
#if DEBUG /* CBMBASIC doesn't do this */
	if (!C) {
		printf("UNIMPL: set top of RAM");
		exit(1);
	}
#endif
	x = RAM_TOP&0xFF;
	y = RAM_TOP>>8;
}

/* MEMBOT */
static void
MEMBOT() {
#if DEBUG /* CBMBASIC doesn't do this */
	if (!C) {
		printf("UNIMPL: set bot of RAM");
		exit(1);
	}
#endif
	x = RAM_BOT&0xFF;
	y = RAM_BOT>>8;
}

/* READST */
static void
READST() {
		a = kernal_status;
}

/* SETLFS */
static void
SETLFS() {
		kernal_lfn = a;
		kernal_dev = x;
		kernal_sec = y;
}

/* SETNAM */
static void
SETNAM() {
		kernal_filename = x | y<<8;
		kernal_filename_len = a;
}

/* OPEN */
static void
OPEN() {
    kernal_status = 0;
    if (kernal_files[kernal_lfn]) {
        set_c(1);
        a = KERN_ERR_FILE_OPEN;
		printf("ERR: KERN_ERR_FILE_OPEN lfn: %d\n", kernal_lfn);
    } else if (kernal_filename_len == 0) {
        set_c(1);
        a = KERN_ERR_MISSING_FILE_NAME;
//		printf("ERR: KERN_ERR_MISSING_FILE_NAME\n");
	} else {
		char filename[41];
		uint8_t len = MIN(kernal_filename_len, sizeof(filename) - 1);
		memcpy(filename, (char *)&RAM[kernal_filename], kernal_filename_len);
		filename[len] = 0;
		printf("OPEN %d,%d,%d,\"%s\"\n", kernal_lfn, kernal_dev, kernal_sec, filename);
		if (kernal_sec == 15) { // command channel
			kernal_files[kernal_lfn] = (void *)-1;
			set_c(0);
		} else {
			char *mode = "r";
			char *comma = strchr(filename, ',');
			if (comma) {
				*comma = 0;
				char type = comma[1]; // 'S'/'P'/'U'/'L' - ignored
				char *comma2 = strchr(comma + 1, ',');
				char mode_c = 0;
				if (comma2) {
					mode_c = comma2[1]; // 'R', 'W'
				}
				if (mode_c == 'W') {
					mode = "w";
				}
//				printf("(%c, %c, %s)\n", type, mode_c, mode);
			}
			kernal_files[kernal_lfn] = fopen(filename, mode);
			if (kernal_files[kernal_lfn]) {
				kernal_files_next[kernal_lfn] = EOF;
				set_c(0);
			} else {
				set_c(1);
				a = KERN_ERR_FILE_NOT_FOUND;
				printf("ERR: KERN_ERR_FILE_NOT_FOUND\n");
			}
		}
    }
}

/* CLOSE */
static void
CLOSE() {
    if (!kernal_files[a]) {
        set_c(1);
        a = KERN_ERR_FILE_NOT_OPEN;
    } else {
        fclose(kernal_files[a]);
        kernal_files[a] = 0;
        set_c(0);
    }
}

/* CHKIN */
static void
CHKIN() {
    kernal_status = 0;
    if (!kernal_files[x]) {
        set_c(1);
        a = KERN_ERR_FILE_NOT_OPEN;
    } else {
        // TODO Check read/write mode
        kernal_input = x;
        set_c(0);
    }
}

/* CHKOUT */
static void
CHKOUT() {
    kernal_status = 0;
    if (!kernal_files[x]) {
        set_c(1);
        a = KERN_ERR_FILE_NOT_OPEN;
    } else {
        // TODO Check read/write mode
        kernal_output = x;
        set_c(0);
    }
}

/* CLRCHN */
static void
CLRCHN() {
    kernal_input = 0;
    kernal_output = 0;
}

static const char run[] = { 'R', 'U', 'N', 13 };

int fakerun = 0;
int fakerun_index = 0;

static const char disk_status[] = "00,OK,00,00\r";
const char *disk_status_p = disk_status;

/* CHRIN */
static void
CHRIN() {
	if (kernal_input != 0) {
		if (kernal_files[kernal_input] == (void *)-1) {
			// command channel
			a = *disk_status_p;
//			printf("%s:%d %c\n", __FILE__, __LINE__, a);
			disk_status_p++;
			if (!*disk_status_p) {
				disk_status_p = disk_status;
			}
			set_c(0);
		} else if (feof(kernal_files[kernal_input])) {
			kernal_status |= KERN_ST_EOF;
			kernal_status |= KERN_ST_TIME_OUT_READ;
			a = 13;
		} else {
			if (kernal_files_next[kernal_input] == EOF)
				kernal_files_next[kernal_input] = fgetc(kernal_files[kernal_input]);
			a = kernal_files_next[kernal_input];
			kernal_files_next[kernal_input] = fgetc(kernal_files[kernal_input]);
			if (kernal_files_next[kernal_input] == EOF)
				kernal_status |= KERN_ST_EOF;
		}
	} else {
		a = getchar(); /* stdin */
		if (a == '\n') a = '\r';
	}
	set_c(0);
}

static void
SCROUT() {
	if (kernal_quote) {
		if (a == '"' || a == '\n' || a == '\r') kernal_quote = 0;
		putchar(a);
	} else {
		switch (a) {
			case 5:
			set_color(COLOR_WHITE);
			break;
			case 10:
			break;
			case 13:
			putchar(13);
			putchar(10);
			break;
			case 17: /* CSR DOWN */
			down_cursor();
			break;
			case 19: /* CSR HOME */
			move_cursor(0, 0);
			break;
			case 28:
			set_color(COLOR_RED);
			break;
			case 29: /* CSR RIGHT */
			right_cursor();
			break;
			case 30:
			set_color(COLOR_GREEN);
			break;
			case 31:
			set_color(COLOR_BLUE);
			break;
			case 129:
			set_color(COLOR_ORANGE);
			break;
			case 144:
			set_color(COLOR_BLACK);
			break;
			case 145: /* CSR UP */
			up_cursor();
			break;
			case 147: /* clear screen */
#ifndef NO_CLRHOME
			clear_screen();
#endif
			break;
			case 149:
			set_color(COLOR_BROWN);
			break;
			case 150:
			set_color(COLOR_LTRED);
			break;
			case 151:
			set_color(COLOR_GREY1);
			break;
			case 152:
			set_color(COLOR_GREY2);
			break;
			case 153:
			set_color(COLOR_LTGREEN);
			break;
			case 154:
			set_color(COLOR_LTBLUE);
			break;
			case 155:
			set_color(COLOR_GREY3);
			break;
			case 156:
			set_color(COLOR_PURPLE);
			break;
			case 158:
			set_color(COLOR_YELLOW);
			break;
			case 159:
			set_color(COLOR_CYAN);
			break;
			case 157: /* CSR LEFT */
			left_cursor();
			break;
			case '"':
			kernal_quote = 1;
			// fallthrough
			default:
			putchar(a);
		}
	}
	fflush(stdout);
	set_c(0);
}

/* CHROUT */
static void
CHROUT() {
#if 0
	int a = *(unsigned short*)(&RAM[0x0100+sp+1]) + 1;
	int b = *(unsigned short*)(&RAM[0x0100+sp+3]) + 1;
	int c = *(unsigned short*)(&RAM[0x0100+sp+5]) + 1;
	int d = *(unsigned short*)(&RAM[0x0100+sp+7]) + 1;
	printf("CHROUT: %d @ %x,%x,%x,%x\n", A, a, b, c, d);
#endif

    if (kernal_output) {
        if (fputc(a, kernal_files[kernal_output]) == EOF) {
            set_c(1);
            a = KERN_ERR_NOT_OUTPUT_FILE;
        } else
            set_c(0);
    } else {
		SCROUT();
	}
}

/* LOAD */
static void
LOAD() {
		FILE *f;
		struct stat st;
		unsigned short start;
		unsigned short end;
		unsigned char savedbyte;

		if (a) {
			printf("UNIMPL: VERIFY\n");
			exit(1);
		}
		if (!kernal_filename_len)
			goto missing_file_name;

/* on special filename $ read directory entries and load they in the basic area memory */
		if( RAM[kernal_filename]=='$' ) {
			DIR *dirp;
			struct dirent *dp;
			int i, file_size;
			unsigned short old_memp, memp = 0x0801;  // TODO hack!

			old_memp = memp;
			memp += 2;
			RAM[memp++] = 0;
			RAM[memp++] = 0;
			RAM[memp++] = 0x12; /* REVERS ON */
			RAM[memp++] = '"';
			for(i=0; i<16; i++)
				RAM[memp+i] = ' ';
			if( (getcwd((char*)&RAM[memp], 256)) == NULL )
				goto device_not_present;
			memp += strlen((char*)&RAM[memp]); /* only 16 on COMMODORE DOS */
			RAM[memp++] = '"';
			RAM[memp++] = ' ';
			RAM[memp++] = '0';
			RAM[memp++] = '0';
			RAM[memp++] = ' ';
			RAM[memp++] = '2';
			RAM[memp++] = 'A';
			RAM[memp++] = 0;

			RAM[old_memp] = (memp) & 0xFF;
			RAM[old_memp+1] = (memp) >> 8;
			
			if ( !(dirp = opendir(".")) )
				goto device_not_present;
			while ((dp = readdir(dirp))) {
				size_t namlen = strlen(dp->d_name);
				stat(dp->d_name, &st);
				file_size = (st.st_size + 253)/254; /* convert file size from num of bytes to num of blocks(254 bytes) */
				if (file_size>0xFFFF)
					file_size = 0xFFFF;
				old_memp = memp;
				memp += 2;
				RAM[memp++] = file_size & 0xFF;
				RAM[memp++] = file_size >> 8;
				if (file_size<1000) {
					RAM[memp++] = ' ';
					if (file_size<100) {
						RAM[memp++] = ' ';
						if (file_size<10) {
							RAM[memp++] = ' ';
						}
					}
				}
				RAM[memp++] = '"';
				if (namlen>16)
					namlen=16; /* TODO hack */
				memcpy(&RAM[memp], dp->d_name, namlen);
				memp += namlen;
				RAM[memp++] = '"';
				for (i=namlen; i<16; i++)
					RAM[memp++] = ' ';
				RAM[memp++] = ' ';
				RAM[memp++] = 'P';
				RAM[memp++] = 'R';
				RAM[memp++] = 'G';
				RAM[memp++] = ' ';
				RAM[memp++] = ' ';
				RAM[memp++] = 0;

				RAM[old_memp] = (memp) & 0xFF;
				RAM[old_memp+1] = (memp) >> 8;
			}
			RAM[memp] = 0;
			RAM[memp+1] = 0;
			(void)closedir(dirp);
			end = memp + 2;
/*
for (i=0; i<255; i++) {
	if (!(i&15))
		printf("\n %04X  ", 0x0800+i);
	printf("%02X ", RAM[0x0800+i]);
}
*/
			goto load_noerr;
		} /* end if( RAM[kernal_filename]=='$' ) */

		savedbyte = RAM[kernal_filename+kernal_filename_len]; /* TODO possible overflow */
		RAM[kernal_filename+kernal_filename_len] = 0;

/* on directory filename chdir on it */
		if( (stat((char*)&RAM[kernal_filename], &st)) == -1 )
			goto file_not_found;
		if(S_ISDIR(st.st_mode)) {
			if( (chdir((char*)&RAM[kernal_filename])) == -1 )
				goto device_not_present;

			RAM[0x0801] = RAM[0x0802] = 0;
			end = 0x0803;
			goto load_noerr;
		}

/* on file load it read it and load in the basic area memory */
		f = fopen((char*)&RAM[kernal_filename], "rb");
		if (!f)
			goto file_not_found;
		start = ((unsigned char)fgetc(f)) | ((unsigned char)fgetc(f))<<8;
		if (!kernal_sec)
			start = x | y<<8;
		end = start + fread(&RAM[start], 1, 65536-start, f); /* TODO may overwrite ROM */
		printf("LOADING FROM $%04X to $%04X\n", start, end);
		fclose(f);

load_noerr:
		x = end & 0xFF;
		y = end >> 8;
		set_c(0);
		a = KERN_ERR_NONE;
		return;
file_not_found:
		set_c(1);
		a = KERN_ERR_FILE_NOT_FOUND;
		return;
device_not_present:
		set_c(1);
		a = KERN_ERR_DEVICE_NOT_PRESENT;
		return;
missing_file_name:
		set_c(1);
		a = KERN_ERR_MISSING_FILE_NAME;
		return;
}

/* SAVE */
static void
SAVE() {
		FILE *f;
		unsigned char savedbyte;
		unsigned short start;
		unsigned short end;

		start = RAM[a] | RAM[a+1]<<8;
		end = x | y << 8;
		if (end<start) {
			set_c(1);
			a = KERN_ERR_NONE;
			return;
		}
		if (!kernal_filename_len) {
			set_c(1);
			a = KERN_ERR_MISSING_FILE_NAME;
			return;
		}
		savedbyte = RAM[kernal_filename+kernal_filename_len]; /* TODO possible overflow */
		RAM[kernal_filename+kernal_filename_len] = 0;
		f = fopen((char*)&RAM[kernal_filename], "wb"); /* overwrite - these are not the COMMODORE DOS semantics! */
		if (!f) {
			set_c(1);
			a = KERN_ERR_FILE_NOT_FOUND;
			return;
		}
		fputc(start & 0xFF, f);
		fputc(start >> 8, f);
		fwrite(&RAM[start], end-start, 1, f);
		fclose(f);
		set_c(0);
		a = KERN_ERR_NONE;
}

/* SETTIM */
static void
SETTIM() {
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
    time_t  now = time(0);
    struct tm       bd;
    struct timeval  tv;
    
    localtime_r(&now, &bd);

    bd.tm_sec       = seconds%60;
    bd.tm_min       = seconds/60;
    bd.tm_hour      = seconds/3600;

    tv.tv_sec   = mktime(&bd);
    tv.tv_usec  = (jiffies % 60) * (1000000/60);
    
    settimeofday(&tv, 0);
#endif
}

/* RDTIM */
static void
RDTIM() {
	unsigned long   jiffies;
#ifdef _WIN32
	SYSTEMTIME st;

	GetLocalTime(&st);
	jiffies = ((st.wHour*60 + st.wMinute)*60 + st.wSecond)*60 + st.wMilliseconds * 60 / 1000;
#else
    time_t  now = time(0);
    struct tm       bd;
    struct timeval  tv;
    
    localtime_r(&now, &bd);
    gettimeofday(&tv, 0);
    
    jiffies = ((bd.tm_hour*60 + bd.tm_min)*60 + bd.tm_sec)*60 + tv.tv_usec / (1000000/60);
#endif
    y   = (unsigned char)(jiffies/65536);
    x   = (unsigned char)((jiffies%65536)/256);
    a   = (unsigned char)(jiffies%256);
    
}

/* STOP */
static void
STOP() {
		set_z(0); /* TODO we don't support the STOP key */
}

/* GETIN */
static void
GETIN() {
    if (kernal_input != 0) {
        if (feof(kernal_files[kernal_input])) {
            kernal_status |= KERN_ST_EOF;
            kernal_status |= KERN_ST_TIME_OUT_READ;
            a = 199;
        } else {
            if (kernal_files_next[kernal_input] == EOF)
                kernal_files_next[kernal_input] = fgetc(kernal_files[kernal_input]);
            a = kernal_files_next[kernal_input];
            kernal_files_next[kernal_input] = fgetc(kernal_files[kernal_input]);
            if (kernal_files_next[kernal_input] == EOF)
                kernal_status |= KERN_ST_EOF;
        }
        set_c(0);
    } else {
#ifdef _WIN32
        if (_kbhit())
            a = _getch();
        else
            a = 0;
#else
        a = getchar();
#endif
        if (a == '\n') a = '\r';
        set_c(0);
    }
}

/* CLALL */
static void
CLALL() {
    int i;
    for (i = 0; i < sizeof(kernal_files)/sizeof(kernal_files[0]); ++i) {
        if (kernal_files[i]) {
            fclose(kernal_files[i]);
            kernal_files[i] = 0;
        }
    }
}

/* PLOT */
static void
PLOT() {
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


/* IOBASE */
static void
IOBASE() {
#define CIA 0xDC00 /* we could put this anywhere... */
		/*
		 * IOBASE is just used inside RND to get a timer value.
		 * So, let's fake this here, too.
		 * Commodore BASIC reads offsets 4/5 and 6/7 to get the
		 * two timers of the CIA.
		 */
		int pseudo_timer;
		pseudo_timer = rand();
		RAM[CIA+4] = pseudo_timer&0xff;
		RAM[CIA+5] = pseudo_timer>>8;
		pseudo_timer = rand(); /* more entropy! */
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

	unsigned int new_pc;
	switch(pc) {
		case 0xE716:	SCROUT();	break;
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
		case 0xFFCF:	CHRIN();	break;
		case 0xFFD2:	CHROUT();	break;
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
	
