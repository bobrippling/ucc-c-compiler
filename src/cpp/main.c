#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "pp.h"

int make_rules = 0;

int main(int argc, char **argv)
{
	const char *inputfname, *outputfname;
	int i, ret, verbose = 0;
	struct pp arg;

	inputfname = outputfname = NULL;

	memset(&arg, 0, sizeof arg);

	adddir(".");

	for(i = 1; i < argc && *argv[i] == '-'; i++){
		if(!strcmp(argv[i]+1, "-"))
			break;

		switch(argv[i][1]){
			case 'I':
				if(argv[i][2])
					adddir(argv[i]+2);
				else
					goto usage;
				break;

			case 'v':
				if(argv[i][2])
					goto usage;
				verbose = 1;
				break;

			case 'o':
				if(outputfname)
					goto usage;

				if(argv[i][2] != '\0')
					outputfname = argv[i] + 2;
				else if(++i < argc)
					outputfname = argv[i];
				else
					goto usage;
				break;

			case 'M':
				if(!strcmp(argv[i] + 2, "M"))
					make_rules = 1;
				else
					goto usage;
				break;

			case 'D':
			{
				char *eq;
				if(!argv[i][2])
					goto usage;

				eq = strchr(argv[i] + 2, '=');
				if(eq){
					*eq++ = '\0';
					ADDDEF(argv[i] + 2, eq);
				}else
					ADDDEF(argv[i] + 2, "");

				break;
			}

			case '\0':
				/* we've been passed "-" as a filename */
				break;

			default:
				goto usage;
		}
	}

	if(i < argc){
		inputfname = argv[i++];
		if(i < argc){
			if(outputfname)
				goto usage;
			outputfname = argv[i++];
			if(i < argc)
				goto usage;
		}
	}

	if(make_rules && !inputfname){
		fprintf(stderr, "%s: can't generate makefile rules with no input\n", *argv);
		return 1;
	}

#define CHECK_FILE(var, file, mode, def) \
	if(var){ \
		if(!strcmp(var, "-")){ \
			arg.file = def; \
		}else{ \
			arg.file = fopen(var, mode); \
			if(!arg.file){ \
				fprintf(stderr, "open: %s: ", var); \
				perror(NULL); \
				return 1; \
			} \
		} \
	}else{ \
		arg.file = def; \
	}

	CHECK_FILE(outputfname, out, "w", stdout)
	CHECK_FILE(inputfname,   in, "r", stdin);

	if(!inputfname)
		inputfname = "stdin";

	arg.fname = inputfname;

	if(verbose)
		for(i = 0; i < argc; i++)
			fprintf(stderr, "argv[%d] = %s\n", i, argv[i]);

	ret = preprocess(&arg, verbose);

	fclose(arg.in);
	if(errno)
		perror("close()");

	fclose(arg.out);
	if(errno)
		perror("close()");

	return ret;
usage:
	fprintf(stderr, "Usage: %s [options] files...\n", *argv);
	fputs(" Options:\n"
				"  -v: verbose\n"
				"  -Idir: Add search directory\n"
				"  -Dxyz: Define xyz\n"
				"  -o output: output file\n", stderr);
	return 1;
}
