
#include <stdio.h>
#include "glue.h"

static FILE *file[4];

void
printer_open(uint8_t dev) {
	char filename[13];
	sprintf(filename, "printer%d.txt", dev);
	file[dev - 4] = fopen(filename, "wb");
}

void
printer_bsout(uint8_t dev) {
	char c = a == '\r' ? '\n' : a;
	fprintf(file[dev - 4], "%c", c);
}

void
printer_close(uint8_t dev) {
	fclose(file[dev - 4]);
}
