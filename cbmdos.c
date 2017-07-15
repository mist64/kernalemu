#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "readdir.h"
#include "glue.h"
#include "error.h"
#include "cbmdos.h"

#define KERN_ST_TIME_OUT_READ 0x02
#define KERN_ST_EOF 0x40

extern void set_c(char f);
extern uint8_t STATUS;

static const char DRIVE_STATUS_00[] = "00, OK,00,00\r";
static const char DRIVE_STATUS_31[] = "31,SYNTAX ERROR,00,00\r";
static const char DRIVE_STATUS_32[] = "32,SYNTAX ERROR,00,00\r";
static const char DRIVE_STATUS_62[] = "62,FILE NOT FOUND,00,00\r";
static const char DRIVE_STATUS_73[] = "73,CBM DOS FOR UNIX,00,00\r";
static const char DRIVE_STATUS_74[] = "74,DRIVE NOT READY,00,00\r";

const char *cur_drive_status;
const char *drive_status_p;

char command[41];
char *command_p;

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

#define FILE_COMMAND_CHANNEL (void *)1
#define FILE_DIRECTORY       (void *)2

uint8_t in_lfn = 0;
uint8_t out_lfn = 0;

uint8_t directory_data[65536];
uint8_t *directory_data_p = directory_data;
uint8_t *directory_data_end;

static void
set_drive_status(const char *drive_status)
{
	cur_drive_status = drive_status;
	drive_status_p = drive_status;
}

void
cbmdos_init()
{
	set_drive_status(DRIVE_STATUS_73);
	command[0] = 0;
	command_p = command;
}

static void
interpret_command()
{
	*command_p = 0;
	char *cr = strchr(command, '\r');
	if (cr) {
		*cr = 0;
//		printf("COMMAND: %s", command);
		switch (command[0]) {
			case 'I':
				set_drive_status(DRIVE_STATUS_00);
				break;
			case 'U':
				switch (command[1]) {
					case 'I':
					case 'J':
						set_drive_status(DRIVE_STATUS_73);
						break;
					default:
						set_drive_status(DRIVE_STATUS_31);
						break;
				}
			default:
				set_drive_status(DRIVE_STATUS_31);
				break;
		}
		command_p = command;
	}
}

static bool
create_directory_listing()
{
	struct stat st;
	DIR *dirp;
	struct dirent *dp;
	int file_size;
	uint16_t end;
	uint16_t old_memp;

	// load address
	*directory_data_p++ = 1;
	*directory_data_p++ = 8;

	// link
	*directory_data_p++ = 1;
	*directory_data_p++ = 1;
	// line number
	*directory_data_p++ = 0;
	*directory_data_p++ = 0;
	*directory_data_p++ = 0x12; /* REVERSE ON */
	*directory_data_p++ = '"';
	for (int i = 0; i < 16; i++) {
		*directory_data_p++ = ' ';
	}
	if (!(getcwd((char *)directory_data_p - 16, 256))) {
		return false;
	}
	*directory_data_p++ = '"';
	*directory_data_p++ = ' ';
	*directory_data_p++ = '0';
	*directory_data_p++ = '0';
	*directory_data_p++ = ' ';
	*directory_data_p++ = '2';
	*directory_data_p++ = 'A';
	*directory_data_p++ = 0;

	if (!(dirp = opendir("."))) {
		return false;
	}
	while ((dp = readdir(dirp))) {
		size_t namlen = strlen(dp->d_name);
		stat(dp->d_name, &st);
		file_size = (st.st_size + 253)/254; /* convert file size from num of bytes to num of blocks(254 bytes) */
		if (file_size > 0xFFFF) {
			file_size = 0xFFFF;
		}

		// link
		*directory_data_p++ = 1;
		*directory_data_p++ = 1;

		*directory_data_p++ = file_size & 0xFF;
		*directory_data_p++ = file_size >> 8;
		if (file_size < 1000) {
			*directory_data_p++ = ' ';
			if (file_size < 100) {
				*directory_data_p++ = ' ';
				if (file_size < 10) {
					*directory_data_p++ = ' ';
				}
			}
		}
		*directory_data_p++ = '"';
		if (namlen > 16) {
			namlen = 16; /* TODO hack */
		}
		memcpy(directory_data_p, dp->d_name, namlen);
		directory_data_p += namlen;
		*directory_data_p++ = '"';
		for (int i = namlen; i < 16; i++) {
			*directory_data_p++ = ' ';
		}
		*directory_data_p++ = ' ';
		*directory_data_p++ = 'P';
		*directory_data_p++ = 'R';
		*directory_data_p++ = 'G';
		*directory_data_p++ = ' ';
		*directory_data_p++ = ' ';
		*directory_data_p++ = 0;
	}
	// link
	*directory_data_p++ = 0;
	*directory_data_p++ = 0;
	(void)closedir(dirp);
	directory_data_end = directory_data_p;
	directory_data_p = directory_data;
	return true;
}

int
cbmdos_open(uint8_t lfn, uint8_t unit, uint8_t sec, const char *filename)
{
	if (sec == 15) { // command channel
		kernal_files[lfn] = FILE_COMMAND_CHANNEL;
		set_c(0);
		if (command_p - command + strlen(filename) > sizeof(command) - 2) {
			set_drive_status(DRIVE_STATUS_32);
		} else {
			strcpy(command_p, filename);
			command_p += strlen(filename);
			*command_p = '\r';
			command_p++;
			interpret_command();
		}
		return true;
	} else {
		if (filename[0] == '$') {
			kernal_files[lfn] = FILE_DIRECTORY;
			if (create_directory_listing()) {
				return KERN_ERR_NONE;
			} else {
				set_drive_status(DRIVE_STATUS_74);
				return KERN_ERR_FILE_NOT_FOUND;
			}
		}

		// remove "0:" etc. prefix
		if (filename[0] >= '0' && filename[0] <= '9' && filename[1] == ':') {
			filename += 2;
		}

		char *mode = sec ? "w" : "r";
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

		if (!strlen(filename)) {
			return KERN_ERR_MISSING_FILE_NAME;
		}

		// TODO: resolve wildcards

//		printf("%s:%d: %s\n",__func__, __LINE__, filename);

		// TODO: "file exists"

		kernal_files[lfn] = fopen(filename, mode);
		if (kernal_files[lfn]) {
			return KERN_ERR_NONE;
		} else {
			set_drive_status(DRIVE_STATUS_62);
			return KERN_ERR_FILE_NOT_FOUND;
		}
	}
}

void
cbmdos_close(uint8_t lfn, uint8_t unit)
{
	switch ((long)kernal_files[lfn]) {
		case (long)FILE_COMMAND_CHANNEL:
			// reset pointer
			drive_status_p = cur_drive_status;
			break;
		case (long)FILE_DIRECTORY:
			break;
		default:
			fclose(kernal_files[lfn]);
			break;
	}
	set_c(0);
	kernal_files[lfn] = 0;
}

void
cbmdos_chkin(uint8_t lfn, uint8_t unit)
{
	// TODO Check read/write mode
	in_lfn = lfn;
}

void
cbmdos_chkout(uint8_t lfn, uint8_t unit)
{
	// TODO Check read/write mode
	out_lfn = lfn;
}

void
cbmdos_basin(uint8_t unit)
{
//	printf("%s:%d LFN: %d\n", __func__, __LINE__, in_lfn);
	if (kernal_files[in_lfn] == FILE_COMMAND_CHANNEL) {
		// command channel
		a = *drive_status_p;
		drive_status_p++;
		if (!*drive_status_p) {
			drive_status_p = cur_drive_status;
		}
	} else if (kernal_files[in_lfn] == FILE_DIRECTORY) {
		a = *directory_data_p++;
		if (directory_data_p == directory_data_end) {
			STATUS |= KERN_ST_EOF;
			STATUS |= KERN_ST_TIME_OUT_READ;
			directory_data_p--;
		}
	} else {
		a = fgetc(kernal_files[in_lfn]);
		if (feof(kernal_files[in_lfn])) {
			STATUS |= KERN_ST_EOF;
		}
	}
	set_c(0);
}

void
cbmdos_bsout(uint8_t unit)
{
	if (kernal_files[out_lfn] == (void *)-1) {
		if (command_p - command > sizeof(command) - 1) {
			set_drive_status(DRIVE_STATUS_32);
			set_c(0);
		} else {
			*command_p = a;
			command_p++;
			interpret_command();
		}
	} else {
		if (fputc(a, kernal_files[out_lfn]) == EOF) {
			set_c(1);
			STATUS = KERN_ERR_NOT_OUTPUT_FILE;
		} else {
			set_c(0);
		}
	}
}
