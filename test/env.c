#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv)
{
	if(argc < 2){
		extern char *__progname;
		extern char **environ;

		char **e;

		for(e = environ; *e; e++)
			printf("%s\n", *e);

		printf("\e[1;35m__progname %s\e[m\n", __progname);

	}else{
		int i;

		for(i = 1; i < argc; i++){
			char *s = argv[i];

			printf("getenv(\"%s\") = ", s);
			s = getenv(s);

			printf("%s%s%s\n",
					s ? "\"" : "",
					s ?   s  : "<not found>",
					s ? "\"" : "");
		}
	}

	return 0;
}
