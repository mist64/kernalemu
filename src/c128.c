// KERNAL Emulator
// Copyright (c) 2009-2021 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "glue.h"
#include "channelio.h"
#include "c128.h"

#define NYI() printf("Unsupported KERNAL call %s at PC=$%04X S=$%02X\n", __func__, pc, sp); exit(1);

// Most of the C128 KERNAL interface additions are currently
// unsupported.

// SPIN_SPOUT - Setup fast serial ports for I/O
void SPIN_SPOUT() { NYI(); }

// CLOSE_ALL - Close all files on a device
void CLOSE_ALL() { NYI(); }

// C64MODE - Reconfigure system as a C64
void C64MODE() { NYI(); }

// DMA_CALL - Send command to DMA device
void DMA_CALL() { NYI(); }

// BOOT_CALL - Boot load program from disk
void BOOT_CALL() { NYI(); }

// PHOENIX - Init function cartridges
void
PHOENIX()
{
	// do nothing
}

// LKUPLA - Search tables for given LA
void LKUPLA() { NYI(); }

// LKUPSA - Search tables for given SA
void LKUPSA() { NYI(); }

// SWAPPER - Switch between 40 and 80 columns
void SWAPPER() { NYI(); }

// DLCHR - Init 80-col character RAM
void DLCHR() { NYI(); }

// PFKEY - Program a function key
void PFKEY() { NYI(); }

// SETBNK - Set bank for I/O operations
void SETBNK() { NYI(); }

// GETCFG - Lookup MMU data for given bank
void GETCFG() { NYI(); }

// JSRFAR - GOSUB in another bank
void JSRFAR() { NYI(); }

// JMPFAR - GOTO another bank
void JMPFAR() { NYI(); }

// INDFET - LDA (fetvec),Y from any bank
void INDFET() { NYI(); }

// INDSTA - STA (stavec),Y to any bank
void INDSTA() { NYI(); }

// INDCMP - CMP (cmpvec),Y to any bank
void INDCMP() { NYI(); }

// PRIMM - Print immediate utility
void
PRIMM()
{
	uint16_t address = (RAM[0x100 + sp + 1] | (RAM[0x100 + sp + 2] << 8)) + 1;
	while ((a = RAM[address++])) {
		BSOUT();
	}
	address--;
	RAM[0x100 + sp + 1] = address & 0xff;
	RAM[0x100 + sp + 2] = address >> 8;
}
