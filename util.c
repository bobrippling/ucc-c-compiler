#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "util.h"
#include "alloc.h"

void die(const char *fmt, ...)
{
	va_list l;

	va_start(l, fmt);
	vfprintf(stderr, fmt, l);
	va_end(l);

	if(fmt[strlen(fmt)-1] == ':')
		perror(NULL);
	else
		fputc('\n', stderr);

	exit(1);
}

char *fline(FILE *f)
{
	int c, pos = 0, len = 10;
	char *line = umalloc(len);

	do{
		if((c = fgetc(f)) == EOF){
			free(line);
			return NULL;
		}

		line[pos++] = c;
		if(pos == len){
			char *tmp;
			len *= 2;
			tmp = realloc(line, len);
			if(!tmp){
				free(line);
				return NULL;
			}
			line = tmp;
			line[pos+1] = '\0';
		}

		if(c == '\n'){
			line[pos-1] = '\0';
			return line;
		}
	}while(1);
}
