#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#define isdigit(x) ('0' <= (x) && (x) <= '9')
#define WRITE(s) write(1, s, strlen(s))

void printd(int n)
{
	n += '0';
	write(1, &n, 1);
}

int timmyprintf(const char *fmt, int n __attribute__((__unused__)))
{

	while(*fmt){
		if(*fmt == '%'){
			int pad = 0;
			fmt++;

			switch(*fmt){
				case 'd':
					while(pad--);
					WRITE("pre-break\n");
					break;
			}
			WRITE("after switch");
		}else{
			write(1, fmt, 1);
		}
		fmt++;
	}
}


main()
{
	//timmyprintf("A%dB\n", 5);
	printf("hello %d\n", 2);
}
