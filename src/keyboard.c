// KERNAL Emulator
// Copyright (c) 2009-2021 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "glue.h"
#include "channelio.h"
#include "keyboard.h"

#define NYI() printf("Unsupported KERNAL call %s at PC=$%04X S=$%02X\n", __func__, pc, sp); exit(1);

// SCNKEY - Scan keyboard
void
SCNKEY()
{
	// This should only be called by custom interrupt handlers.
	// do nothing
}

// STOP - Scan stop key
void
STOP()
{
	// TODO: We don't support the STOP key
	set_z(0);
}

// GETIN - Get a character
void
GETIN()
{
	BASIN();
}
