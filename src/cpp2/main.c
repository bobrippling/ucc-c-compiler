#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "macro.h"
#include "preproc.h"

static const struct
{
	const char *nam, *val;
} initial_defs[] = {
	{ "__unix__", "1"  },
	{ NULL,       NULL }
};

const char *current_fname;

int main(int argc, char **argv)
{
	const char *infname, *outfname;
	int ret = 0;
	int i;

	infname = outfname = NULL;

	for(i = 1; i < argc && *argv[i] == '-'; i++){
		if(!strcmp(argv[i]+1, "-"))
			break;

		switch(argv[i][1]){
			case 'I':
				if(argv[i][2])
					macro_add_dir(argv[i]+2);
				else
					goto usage;
				break;

			case 'o':
				if(outfname)
					goto usage;

				if(argv[i][2])
					outfname = argv[i] + 2;
				else if(++i < argc)
					outfname = argv[i];
				else
					goto usage;
				break;

			case 'M':
				if(!strcmp(argv[i] + 2, "M")){
					fprintf(stderr, "TODO\n");
					return 1;
				}else{
					goto usage;
				}
				break;

			case 'D':
			{
				char *eq;
				if(!argv[i][2])
					goto usage;

				eq = strchr(argv[i] + 2, '=');
				if(eq){
					*eq++ = '\0';
					macro_add(argv[i] + 2, eq);
				}else{
					macro_add(argv[i] + 2, "");
				}
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
		infname = argv[i++];
		if(i < argc){
			if(outfname)
				goto usage;
			outfname = argv[i++];
			if(i < argc)
				goto usage;
		}
	}

#define CHECK_FILE(var, mode, target) \
	if(var && strcmp(var, "-")){ \
		if(!freopen(var, mode, target)){ \
			fprintf(stderr, "open: %s: ", var); \
			perror(NULL); \
			return 1; \
		} \
	}

	CHECK_FILE(outfname, "w", stdout)
	CHECK_FILE(infname,  "r", stdin)

	if(!infname)
		infname = "stdin";

	current_fname = infname;

	preprocess();

	fclose(stdin);
	if(errno){
		perror("close()");
		ret = 1;
	}

	fclose(stdout);
	if(errno){
		perror("close()");
		ret = 1;
	}

	return ret;
usage:
	fprintf(stderr, "Usage: %s [options] files...\n", *argv);
	fputs(" Options:\n"
				"  -Idir: Add search directory\n"
				"  -Dxyz: Define xyz\n"
				"  -o output: output file\n", stderr);
	return 1;
}
