// KERNAL Emulator
// Copyright (c) 2009-2021 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "glue.h"
#include "ieee488.h"

#define NYI() printf("Unsupported KERNAL call %s at PC=$%04X S=$%02X\n", __func__, pc, sp); exit(1);

// The low-level IEEE-488/IEC calls are currently
// unsupported. Few applications need them, and it
// is not clear what mapping these to modern systems
// would mean in the first place.

void SECOND() { NYI(); }
void TKSA() { NYI(); }
void SETTMO() { NYI(); }
void ACPTR() { NYI(); }
void CIOUT() { NYI(); }
void UNTLK() { NYI(); }
void UNLSN() { NYI(); }
void LISTEN() { NYI(); }
void TALK() { NYI(); }
