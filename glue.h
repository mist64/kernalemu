#include <stdint.h>

extern uint8_t a, x, y, sp, status;
extern uint16_t pc;
extern uint8_t RAM[65536];

typedef enum {
	MACHINE_PET,
	MACHINE_PET4,
	MACHINE_VIC20,
	MACHINE_C64,
	MACHINE_C128,
} machine_t;

__unused static void
set_c(char f)
{
	status = (status & ~1) | !!f;
}

__unused static void
set_z(char f)
{
	status = (status & ~2) | (!!f << 1);
}
