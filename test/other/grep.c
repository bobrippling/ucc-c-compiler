#include <stdio.h>

#ifndef BUFSIZ
#  define BUFSIZ 256
#endif

int grep(char *regexp, FILE *f, char *name)
{
	int n, nmatch;
	char buf[BUFSIZ];

	nmatch = 0;
	while(fgets(buf, sizeof buf, f)){
		n = strlen(buf);
		if(n > 0 && buf[n-1] == '\n')
			buf[n-1] = '\0';
		if(match(regexp, buf)){
			nmatch++;

			if(name)
				printf("%s:", name);
			printf("%s\n", buf);
		}
	}
	return nmatch;
}

int matchhere(char *regexp, char *text)
{
	if(regexp[0] == '\0')
		return 1;
	if(regexp[1] == '*')
		return matchstar(regexp[0], regexp + 2, text);
	if(regexp[0] == '$' && regexp[1] == '\0')
		return *text == '\0';
	if(*text != '\0' && (regexp[0] == '.' || regexp[0] == *text))
		return matchhere(regexp+1, text+1);
	return 0;
}

int match(char *regexp, char *text)
{
	do{
		if(matchhere(regexp, text))
			return 1;
	}while(*text++ != '\0');
	return 0;
}

int matchstar(int c, char *regexp, char *text)
{
	do{
		if(matchhere(regexp, text))
			return 1;
	}while(*text != '\0' && (*text++ == c || c == '.'));
	return 0;
}
