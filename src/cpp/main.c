#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pp.h"

int make_rules = 0;

int main(int argc, const char **argv)
{
	const char *inputfilename = NULL,
	           *outputfilename = NULL;
	char argv_options = 1;
	int i, ret, verbose = 0;
	struct pp arg;

	memset(&arg, 0, sizeof arg);

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
						goto usage;
					break;

				case 'v':
					if(argv[i][2])
						goto usage;
					verbose = 1;
					break;

				case 'o':
					if(outputfilename)
						goto usage;

					if(argv[i][2] != '\0')
						outputfilename = argv[i] + 2;
					else if(++i < argc)
						outputfilename = argv[i];
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
						adddef(argv[i] + 2, eq);
					}else
						adddef(argv[i] + 2, "");

					break;
				}

				case '\0':
					/* we've been passed "-" as a filename */
					break;

				default:
					goto usage;
			}
		}else{
			break;
		}

	if(make_rules && !inputfilename){
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

	CHECK_FILE(outputfilename, out, "w", stdout)

	ret = 0;

	for(; i < argc; i++){
		const char *inputfilename = argv[i];

		CHECK_FILE(inputfilename, in, "r", stdin)

		arg.fname = inputfilename ? inputfilename : "-";

		ret |= preprocess(&arg, verbose);

		if(fclose(arg.in) == EOF)
			perror("fclose()");
		if(fclose(arg.out) == EOF)
			perror("fclose()");
	}

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
