#include <stdio.h>
#include <stdlib.h>
#include <sys/utsname.h>

#include "utils.h"

/*
 * NULL on error or EOF,
 */
char *readline(FILE *f)
{
#define LINE_STEP 10
	int c, pos = 0, len = LINE_STEP;
	char *line = malloc(len);

	if(!line)
		return NULL;

	do{
		switch(c = fgetc(f)){
			case '\b': /* backspace */
				if(pos > 0)
					line[pos--] = '\0';
				break;

			case EOF:
				free(line);
				return NULL;


			default:
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
					line[pos] = '\0';
#if TOKENREAD_DEBUG
					fprintf(stderr, "> %s", line);
#endif
					return line;
				}

		}
	}while(1);
#undef LINE_STEP
}

enum archtype getarch()
{
	struct utsname un;

	if(uname(&un) == -1)
		perror("uname()");
	else if(!strcmp("i686", un.machine))
		return ARCH_32;
	else if(!strcmp("x86_64", un.machine))
		return ARCH_64;
	else
		fprintf(stderr, "unrecognised machine architecture: \"%s\"\n", un.machine);

	return ARCH_UNKNOWN;
}
