#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

#include "pp.h"

jmp_buf allocerr;

void usage(const char *);
void adddir(const char *);

void usage(const char *name)
{
	fprintf(stderr, "Usage: %s [options] [file]\n", name);
	fputs(" Options:\n"
				"  -v: verbose\n"
				"  -Idir: Add search directory\n"
				"  -Dxyz: Define xyz\n"
				"  -o output: output file\n", stderr);
	exit(1);
}

int main(int argc, const char **argv)
{
#define USAGE() usage(*argv)
	const char *inputfilename = NULL,
	           *outputfilename = NULL;
	char argv_options = 1;
	int i, ret, verbose = 0;
	struct pp arg;

	memset(&arg, 0, sizeof arg);

	if(setjmp(allocerr)){
		perror("malloc()");
		return 2;
	}

	adddir(".");

	for(i = 1; i < argc; i++)
		if(argv_options && *argv[i] == '-'){
			if(!strcmp(argv[i]+1, "-")){
				argv_options = 0;
				continue;
			}

			switch(argv[i][1]){
				case 'I':
					if(argv[i][2])
						adddir(argv[i]+2);
					else
						USAGE();
					break;

				case 'v':
					if(argv[i][2])
						USAGE();
					verbose = 1;
					break;

				case 'o':
					if(outputfilename)
						USAGE();

					if(argv[i][2] != '\0')
						outputfilename = argv[i] + 2;
					else if(++i < argc)
						outputfilename = argv[i];
					else
						USAGE();
					break;

				case 'D':
				{
					char *eq;
					if(!argv[i][2])
						USAGE();

					eq = strchr(argv[i] + 2, '=');
					if(eq){
						*eq++ = '\0';
						adddef(argv[i] + 2, eq);
					}else
						adddef(argv[i] + 2, "");

					break;
				}

				default:
					USAGE();
			}

		}else if(!inputfilename)
			inputfilename = argv[i];
		else
			USAGE();


#define CHECK_FILE(var, file, mode, def) \
	if(var){ \
		if(!strcmp(var, "-")) \
			arg.file = def; \
		else{ \
			arg.file = fopen(var, mode); \
			if(!arg.file){ \
				fprintf(stderr, "open: %s: ", var); \
				perror(NULL); \
				return 1; \
			} \
		} \
	}else \
		arg.file = def;

	CHECK_FILE(inputfilename,  in,  "r", stdin)
	CHECK_FILE(outputfilename, out, "w", stdout)

	arg.fname = inputfilename ? inputfilename : "-";

	ret = preprocess(&arg, verbose);

	if(fclose(arg.in) == EOF)
		perror("fclose()");
	if(fclose(arg.out) == EOF)
		perror("fclose()");

	return ret;
}
