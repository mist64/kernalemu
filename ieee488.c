#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "glue.h"
#include "ieee488.h"

#define NYI() printf("Unsupported KERNAL call %s at PC=$%04X S=$%02X\n", __func__, pc, sp); exit(1);

void SECOND() { NYI(); }
void TKSA() { NYI(); }
void SETTMO() { NYI(); }
void ACPTR() { NYI(); }
void CIOUT() { NYI(); }
void UNTLK() { NYI(); }
void UNLSN() { NYI(); }
void LISTEN() { NYI(); }
void TALK() { NYI(); }
