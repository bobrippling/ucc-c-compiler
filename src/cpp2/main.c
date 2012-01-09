#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>

#include "macro.h"
#include "preproc.h"
#include "../util/util.h"
#include "../util/dynarray.h"
#include "../util/alloc.h"
#include "../util/platform.h"

static const struct
{
	const char *nam, *val;
} initial_defs[] = {
	{ "__unix__", "1"  },
	{ "__FILE__", NULL },
	{ "__LINE__", NULL },
	{ NULL,       NULL }
};

const char *current_fname;

char **dirnames = NULL;
int debug = 0;

void dirname_push(char *d)
{
	/*fprintf(stderr, "dirname_push(%s = %p)\n", d, d);*/
	dynarray_add((void ***)&dirnames, d);
}

char *dirname_pop()
{
	char *r = dynarray_pop((void **)dirnames);
	/*fprintf(stderr, "dirname_pop() = %s (%p)\n", r, r);
	return r; TODO - free*/
	return NULL;
}

int main(int argc, char **argv)
{
	const char *infname, *outfname;
	int ret = 0;
	int i;

	infname = outfname = NULL;

	for(i = 0; initial_defs[i].nam; i++)
		macro_add(initial_defs[i].nam, initial_defs[i].val);

	if(platform_type() == PLATFORM_64)
		macro_add("__x86_64__", "1");

	switch(platform_sys()){
#define MAP(t, s) case t: macro_add(s, "1"); break
		MAP(PLATFORM_LINUX,   "__linux__");
		MAP(PLATFORM_FREEBSD, "__FreeBSD__");
		MAP(PLATFORM_DARWIN,  "__DARWIN__");
#undef MAP
	}

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

			case 'd':
				debug++;
				break;

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

	if(infname){
		dirname_push(udirname(infname));
	}else{
		infname = "<stdin>";
		dirname_push(ustrdup("."));
	}

	current_fname = infname;

	if(DEBUG_VERB < debug){
		extern macro **macros;
		for(i = 0; macros[i]; i++)
			fprintf(stderr, "### macro \"%s\" = \"%s\"\n",
					macros[i]->nam, macros[i]->val);
	}

	preprocess();

	free(dirname_pop());

	fclose(stdout);
	if(errno)
		die("close():");

	return ret;
usage:
	fprintf(stderr, "Usage: %s [options] files...\n", *argv);
	fputs(" Options:\n"
				"  -Idir: Add search directory\n"
				"  -Dxyz: Define xyz\n"
				"  -o output: output file\n"
				"  -d: increase debug tracing\n", stderr);
	return 1;
}
