void cbmdos_init();
int cbmdos_open(uint8_t lfn, uint8_t unit, uint8_t sec, const char *filename);
void cbmdos_close(uint8_t lfn, uint8_t unit);
void cbmdos_chkin(uint8_t lfn, uint8_t unit);
void cbmdos_chkout(uint8_t lfn, uint8_t unit);
void cbmdos_basin(uint8_t unit);
void cbmdos_bsout(uint8_t unit);
