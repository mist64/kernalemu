#include <stdio.h>
#include <stdlib.h>
#include "console.h"
#include "glue.h"

#define NO_CLRHOME

static int kernal_quote = 0;

// CINT - Initialize screen editor and devices
void
CINT()
{
	// do nothing
}

// SCREEN - Return screen format
void
SCREEN()
{
	// TODO: return actual values
	x = 80;
	y = 25;
}

// PLOT
void
PLOT() // Read/set X,Y cursor position
{
	if (status & 1) {
		int CX, CY;
		get_cursor(&CX, &CY);
		y = CX;
		x = CY;
	} else {
		printf("UNIMPL: set cursor %d %d\n", y, x);
		exit(1);
	}
}

void
screen_bsout()
{
	if (kernal_quote) {
		if (a == '"' || a == '\n' || a == '\r') {
			kernal_quote = 0;
		}
		putchar(a);
	} else {
		switch (a) {
			case 5:
				set_color(COLOR_WHITE);
				break;
			case 10:
				break;
			case 13:
				putchar(13);
				putchar(10);
				break;
			case 17: /* CSR DOWN */
				down_cursor();
				break;
			case 19: /* CSR HOME */
				move_cursor(0, 0);
				break;
			case 28:
				set_color(COLOR_RED);
				break;
			case 29: /* CSR RIGHT */
				right_cursor();
				break;
			case 30:
				set_color(COLOR_GREEN);
				break;
			case 31:
				set_color(COLOR_BLUE);
				break;
			case 129:
				set_color(COLOR_ORANGE);
				break;
			case 144:
				set_color(COLOR_BLACK);
				break;
			case 145: /* CSR UP */
				up_cursor();
				break;
			case 147: /* clear screen */
#ifndef NO_CLRHOME
				clear_screen();
#endif
				break;
			case 149:
				set_color(COLOR_BROWN);
				break;
			case 150:
				set_color(COLOR_LTRED);
				break;
			case 151:
				set_color(COLOR_GREY1);
				break;
			case 152:
				set_color(COLOR_GREY2);
				break;
			case 153:
				set_color(COLOR_LTGREEN);
				break;
			case 154:
				set_color(COLOR_LTBLUE);
				break;
			case 155:
				set_color(COLOR_GREY3);
				break;
			case 156:
				set_color(COLOR_PURPLE);
				break;
			case 158:
				set_color(COLOR_YELLOW);
				break;
			case 159:
				set_color(COLOR_CYAN);
				break;
			case 157: /* CSR LEFT */
				left_cursor();
				break;
			case '"':
				kernal_quote = 1;
				// fallthrough
			default:
				putchar(a);
		}
	}
	fflush(stdout);
}
