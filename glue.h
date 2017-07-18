#include <stdint.h>

extern uint8_t a, x, y, sp, status;
extern uint16_t pc;
extern uint8_t RAM[65536];

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
