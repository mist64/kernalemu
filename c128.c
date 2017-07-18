#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "glue.h"
#include "c128.h"

#define NYI() printf("Unsupported KERNAL call %s at PC=$%04X S=$%02X\n", __func__, pc, sp); exit(1);

void SPIN_SPOUT() { NYI(); }
void CLOSE_ALL() { NYI(); }
void C64MODE() { NYI(); }
void DMA_CALL() { NYI(); }
void BOOT_CALL() { NYI(); }
void PHOENIX() { NYI(); }
void LKUPLA() { NYI(); }
void LKUPSA() { NYI(); }
void SWAPPER() { NYI(); }
void DLCHR() { NYI(); }
void PFKEY() { NYI(); }
void SETBNK() { NYI(); }
void GETCFG() { NYI(); }
void JSRFAR() { NYI(); }
void JMPFAR() { NYI(); }
void INDFET() { NYI(); }
void INDSTA() { NYI(); }
void INDCMP() { NYI(); }
void PRIMM() { NYI(); }
