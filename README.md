# kernalemu - Commodore KERNAL emulator

**kernalemu** is a C reimplementation of the Commodore KERNAL API, combined with a 6502 emulator. It allows runnung cleanly written PET/C64/C128 etc. command line applications on the UNIX command line, like BASIC interpreters, assemblers, monitors and text adventures.

## Usage

    kernalemu [options] file...
   
You have to specify one or more programs. They have to be in PRG format, i.e. the first two bytes of the file contain the load address. All files are loaded into the emulator's memory.

### Options

* `-start <addr>`: entry point
* `-startind <addr>`: location of entry point vector
* `-machine <machine>`: KERNAL API level, valid arguments are `pet` (v1/2/3), `pet4` (v4), `c64`, `ted` (C16, C116, Plus/4), `c128`, `c65` (default: `c64`)
* `-text`: assume PETSCII upper/lower; convert to ASCII
* `-graphics`: assume PETSCII upper/graphics; do not convert (default)
* `-columns <n>`: insert hard line breaks after <n> columns

### Comments

* If no machine is specified, it is autodetected from the load address.
* If no entry point is specified:
	* If the program has a `SYS` BASIC header, the entry point is extracted from it automatically.
	* Otherwise, it defaults to the load address.

## Examples

There are several applications in the `demo` subdirectory. Here is how to run them:

### PET BASIC V4

	$ kernalemu demo/basic4.prg -start 0xd3b5 -machine pet4
    
	*** COMMODORE BASIC 4.0 ***
	
	 31743 BYTES FREE
	
	READY.

### VIC-20 BASIC V2

	$ kernalemu demo/vic20basic.prg -startind 0xc000 -machine vic20
	
	**** CBM BASIC V2 ****
	38911 BYTES FREE
	
	READY.

### VIC-20 BASIC V2 + BASIC V4

	$ kernalemu demo/vic20basic.prg demo/vic20basic4.prg -startind 0xa000 -machine vic20

	**** CBM BASIC V2 ****
	38911 BYTES FREE
	
	READY.

### C64 BASIC V2

	$ kernalemu demo/basic2.prg -startind 0xa000 -machine c64
	
	
	    **** COMMODORE 64 BASIC V2 ****
	
	 64K RAM SYSTEM  38911 BASIC BYTES FREE
	
	READY.

### C64 BASIC V2 + SIMONS BASIC

	$ kernalemu demo/simonsbasic.prg demo/basic2.prg -startind 0x8000

	*** EXTENDED CBM V2 BASIC ***
	
	38911 BASIC BYTES FREE
	
	READY.

### Plus/4 BASIC V3.5

	$ kernalemu demo/c16basic.prg -start 0x8000 -machine 264

	?OUT OF MEMORY ERROR IN 0
	READY.

*TODO*

### C128 BASIC V7

	$ kernalemu demo/c128basic.prg -start 0x4000 -machine c128

	
	 COMMODORE BASIC V7.0 122365 BYTES FREE
	   (C)1986 COMMODORE ELECTRONICS, LTD.
		 (C)1977 MICROSOFT CORP.
		   ALL RIGHTS RESERVED
	
	READY.

### Assembler 64

	$ kernalemu demo/assembler64.prg
	
	
	CBM RESIDENT ASSEMBLER V080282
	(C) 1982 BY COMMODORE BUSINESS MACHINES
	
	OBJECT FILE (CR OR D:NAME): 

This is the assembler Commodore used to build their ROMs. It will successfully build e.g. [the C64 KERNAL source](https://www.pagetable.com/?p=894) if you extract the files from the disk image and pass "kernal" as the source file name:

	echo "\r\r\rkernal" | kernalemu assembler64.prg

	[...]
	01727  EE13              ;INPUT A BYTE FROM SERIAL BUS
	01728  EE13              ;
	01729  EE13              ACPTR
	01730  EE13  78                 SEI             ;NO IRQ ALLOWED
	01731  EE14  A9 00              LDA #$00        ;SET EOI/ERROR FLAG
	01732  EE16  85 A5              STA COUNT
	01733  EE18  20 85 EE           JSR CLKHI       ;MAKE SURE CLOCK LINE IS RELEASED
	01734  EE1B  20 A9 EE    ACP00A JSR DEBPIA      ;WAIT FOR CLOCK HIGH
	01735  EE1E  10 FB              BPL ACP00A
	01736  EE20              ;
	01737  EE20              EOIACP
	01738  EE20  A9 01              LDA #$01        ;SET TIMER 2 FOR 256US
	01739  EE22  8D 07 DC           STA D1T2H
	01740  EE25  A9 19              LDA #TIMRB
	[...]

### Assembler 128

	$ kernalemu demo/assembler128.prg -start 0x2000 -machine c128

	C/128 6502 ASSEMBLER V022086
	(C)1986 BY COMMODORE BUSINESS MACHINES
	
	
	SOURCE FILE  (SYNTAX: [DRIVE:]FILENAME) ? 

*TODO*

### VIC-Mon for VIC-20

	$ kernalemu demo/vicmon.prg -startind 0x4002 -machine vic20

	VIC20 MICROMON V1.2   BILL YEE 22 JAN  83
	B*
	    PC  IRQ  SR AC XR YR SP
	.; 404D 4391 22 33 00 00 FF
	.

### Commodore Monitor for C64

	$ kernalemu demo/monitor64.prg -machine c64
	
	B*
	   PC  SR AC XR YR SP
	.;C03D 22 00 C3 00 FF
	.

### [Final Cartridge III Monitor](https://github.com/mist64/final_cartridge)

	$ kernalemu demo/fc3monitor.prg -machine c64
	
	C*
	   PC  IRQ  BK AC XR YR SP NV#BDIZC
	.;4024 0000 07 8D FF 00 FF *.*.....
	.


### [Scott Adams Text Adventures](http://www.zimmers.net/anonftp/pub/cbm/vic20/games/16k/Scott%20Adams/index.html)

	$ kernalemu 1.Adventure\ Land.prg -machine vic20 -start 0x7F50

	COMMODORE Presents:
	
	***** ADVENTURE *****
	
	(C) 1981
	by Scott Adams,INC
	
	Adventure number: 1
	
	Version: 1/416
	
	Want to restore
	previously saved game?

## Credits

This repository is maintained by Michael Steil, <mist64@mac.com>
