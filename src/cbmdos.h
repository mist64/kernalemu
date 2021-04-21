// KERNAL Emulator
// Copyright (c) 2009-2021 Michael Steil
// All rights reserved. License: 2-clause BSD

void cbmdos_init();
int cbmdos_open(uint8_t lfn, uint8_t unit, uint8_t sec, const char *filename);
void cbmdos_close(uint8_t lfn, uint8_t unit);
void cbmdos_chkin(uint8_t lfn, uint8_t unit);
void cbmdos_chkout(uint8_t lfn, uint8_t unit);
uint8_t cbmdos_basin(uint8_t unit, uint8_t *status);
int cbmdos_bsout(uint8_t unit, uint8_t c);
