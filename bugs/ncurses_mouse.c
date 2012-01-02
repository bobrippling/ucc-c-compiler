#include <stdio.h>
#include <termios.h>
#include <unistd.h>
int main(void)
{
	unsigned char c;
	int state = 0;
	unsigned char pattern[3] = { 0x1b, '[', 'M' };
	unsigned char mods, col, row;
	struct termios t;
	printf("\033[?1000h");

	tcgetattr(0, &t);
	t.c_lflag &= ~(ICANON|ECHO);
	tcsetattr(0, TCSANOW, &t);
	fflush(stdout);

	while (read(0, &c, 1) == 1) {
		switch(state) {
		case 0:
		case 1:
		case 2:
			if (c == pattern[state])
				state++;
			else
				state = 0;
			break;
		case 3:
			mods = c;
			state++;
			break;
		case 4:
			col = c - 33;
			state++;
			break;
		case 5:
			row = c - 33;
			state = 0;
			printf("Got mouse click %02x %d %d\n", mods, row, col);
			break;
		}
		if (state == 0 && c == 'x')
			break;
	}

	printf("\033[?1000l");
	fflush(stdout);
	t.c_lflag |= ICANON|ECHO;
	tcsetattr(0, TCSANOW, &t);
}
