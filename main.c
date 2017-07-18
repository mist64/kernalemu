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
#include "dispatch.h"

static uint16_t
parse_num(char *s)
{
	if (s[0] == '$') {
		s++;
	} else if (s[0] == '0' && s[1] == 'x') {
		s += 2;
	}
	return strtoul(s, NULL, 16);
}
int
main(int argc, char **argv)
{
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
	machine_t machine;

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

	kernal_init();

	for (;;) {
//		printf("pc = %04x; %02x %02x %02x\n", pc, RAM[pc], RAM[pc+1], RAM[pc+2]);
		step6502();
		if (!RAM[pc]) {
			kernal_dispatch(machine);
		}
	}

	return 0;
}

