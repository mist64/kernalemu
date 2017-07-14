extern unsigned char RAM[65536];

extern unsigned char a, x, y, sp, status;
extern unsigned short pc;

#define STACK16(i) (RAM[0x0100+i]|(RAM[0x0100+i+1]<<8))
#define PUSH(b) RAM[0x0100+S--] = (b)
#define PUSH_WORD(b)      PUSH((b)>>8); PUSH((b)&0xFF)
