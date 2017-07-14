#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "glue.h"
#include "error.h"
#include "cbmdos.h"

#define KERN_ST_TIME_OUT_READ 0x02
#define KERN_ST_EOF 0x40

extern void set_c(char f);
extern uint8_t kernal_status;

static const char disk_status[] = "00, OK,00,00\r";
const char *disk_status_p = disk_status;

//00,OK,00,00
//01,FILES SCRATCHED,XX,00
//20,READ ERROR,TT,SS
//21,READ ERROR,TT,SS
//22,READ ERROR,TT,SS
//23,READ ERROR,TT,SS
//24,READ ERROR,TT,SS
//25,WRITE ERROR,TT,SS
//26,WRITE PROTECT ON,TT,SS
//27,READ ERROR,TT,SS
//28,WRITE ERROR,TT,SS
//29,DISK ID MISMATCH,TT,SS
//30,SYNTAX ERROR,00,00
//31,SYNTAX ERROR,00,00
//32,SYNTAX ERROR,00,00
//33,SYNTAX ERROR,00,00
//34,SYNTAX ERROR,00,00
//39,FILE NOT FOUND,00,00
//50,RECORD NOT PRESENT,00,00
//51,OVERFLOW IN RECORD,00,00
//52,FILE TOO LARGE,00,00
//60,WRITE FILE OPEN,00,00
//61,FILE NOT OPEN,00,00
//62,FILE NOT FOUND,00,00
//63,FILE EXISTS,00,00
//64,FILE TYPE MISMATCH,00,00
//65,NO BLOCK,TT,TT
//66,ILLEGAL TRACK OR SECTOR,TT,SS
//67,ILLEGAL TRACK OR SECTOR,TT,SS
//70,NO CHANNEL,00,00
//71,DIR ERROR,TT,SS
//72,DISK FULL,00,00
//73,CBM DOS V2.6 1541,00,00
//74,DRIVE NOT READY,00,00

FILE *kernal_files[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
int kernal_files_next[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

uint8_t in_lfn = 0, out_lfn = 0;

void
cbmdos_open(uint8_t lfn, uint8_t dev, uint8_t sec, const char *filename)
{
	if (sec == 15) { // command channel
		kernal_files[lfn] = (void *)-1;
		set_c(0);
	} else {
		if (!strlen(filename)) {
			set_c(1);
			a = KERN_ERR_MISSING_FILE_NAME;
			return;
		}

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
		kernal_files[lfn] = fopen(filename, mode);
		if (kernal_files[lfn]) {
			kernal_files_next[lfn] = EOF;
			set_c(0);
		} else {
			set_c(1);
			a = KERN_ERR_FILE_NOT_FOUND;
		}
	}
}

void
cbmdos_close(uint8_t lfn, uint8_t dev)
{
	fclose(kernal_files[lfn]);
	kernal_files[lfn] = 0;
}

void
cbmdos_chkin(uint8_t lfn, uint8_t dev)
{
	// TODO Check read/write mode
	in_lfn = lfn;
}

void
cbmdos_chkout(uint8_t lfn, uint8_t dev)
{
	// TODO Check read/write mode
	out_lfn = lfn;
}

void
cbmdos_basin(uint8_t dev)
{
//	printf("%s:%d LFN: %d\n", __func__, __LINE__, in_lfn);
	if (kernal_files[in_lfn] == (void *)-1) {
		// command channel
		a = *disk_status_p;
		disk_status_p++;
		if (!*disk_status_p) {
			disk_status_p = disk_status;
		}
		set_c(0);
	} else if (feof(kernal_files[in_lfn])) {
		kernal_status |= KERN_ST_EOF;
		kernal_status |= KERN_ST_TIME_OUT_READ;
		set_c(0);
		a = 13;
	} else {
		a = fgetc(kernal_files[in_lfn]);
		if (feof(kernal_files[in_lfn])) {
			kernal_status |= KERN_ST_EOF;
		}
		set_c(0);
	}
}

void
cbmdos_bsout(uint8_t dev, uint8_t c)
{
	if (fputc(c, kernal_files[out_lfn]) == EOF) {
		set_c(1);
		a = KERN_ERR_NOT_OUTPUT_FILE;
	} else {
		set_c(0);
	}
}
