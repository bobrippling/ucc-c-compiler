#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <time.h>

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
	/* standard */
	{ "__unix__",       "1"  },
	/* __STDC__ TODO */

	{ "__SIZE_TYPE__",    "unsigned long"  },
	{ "__PTRDIFF_TYPE__", "unsigned long"  },

	{ "__GOT_SHORT_LONG", "1"  },

	/* non-standard */
	{ "__BLOCKS__",     "1"  },

	/* custom */
	{ "__UCC__",        "1"  },

	/* magic */
	{ "__FILE__",       NULL },
	{ "__LINE__",       NULL },
	{ "__COUNTER__",    NULL },
	{ "__DATE__",       NULL },
	{ "__TIME__",       NULL },

	{ NULL,             NULL }
};

const char *current_fname, *current_line_str;
int show_current_line = 1;

char cpp_time[16], cpp_date[16];

char **dirnames = NULL;

int option_debug     = 0;
int option_line_info = 1;


void dirname_push(char *d)
{
	/*fprintf(stderr, "dirname_push(%s = %p)\n", d, d);*/
	dynarray_add((void ***)&dirnames, d);
}

char *dirname_pop()
{
	char *r = dynarray_pop((void ***)&dirnames);
	(void)r;
	/*fprintf(stderr, "dirname_pop() = %s (%p)\n", r, r);
	return r; TODO - free*/
	return NULL;
}

void calctime(void)
{
	time_t t;
	struct tm *now;

	t = time(NULL);
	now = localtime(&t);

	if(!now)
		die("localtime():");

#define FTIME(s, fmt) \
	if(!strftime(s, sizeof s, fmt, now)) \
		die("strftime():");

	FTIME(cpp_time, "\"%H:%M:%S\"");
	FTIME(cpp_date, "\"%b %d %G\"");
}


int main(int argc, char **argv)
{
	const char *infname, *outfname;
	int ret = 0;
	int i;

	infname = outfname = NULL;

	for(i = 0; initial_defs[i].nam; i++)
		macro_add(initial_defs[i].nam, initial_defs[i].val);

	switch(platform_type()){
		case PLATFORM_x86_64:
			macro_add("__LP64__", "1");
			macro_add("__x86_64__", "1");
			break;

		case PLATFORM_mipsel_32:
			macro_add("__MIPS__", "1");
	}

	switch(platform_sys()){
#define MAP(t, s) case t: macro_add(s, "1"); break
		MAP(PLATFORM_LINUX,   "__linux__");
		MAP(PLATFORM_FREEBSD, "__FreeBSD__");
		MAP(PLATFORM_DARWIN,  "__DARWIN__");
		MAP(PLATFORM_CYGWIN,  "__CYGWIN__");
#undef MAP
	}

	calctime();

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

			case 'L':
				option_line_info = 0;
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
					macro_add(argv[i] + 2, "1"); /* -Dhello means #define hello 1 */
				}
				break;
			}

			case 'U':
				if(!argv[i][2])
					goto usage;
				macro_remove(argv[i] + 2);
				break;

			case 'd':
				option_debug++;
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

	if(DEBUG_VERB < option_debug){
		extern macro **macros;
		for(i = 0; macros[i]; i++)
			fprintf(stderr, "### macro \"%s\" = \"%s\"\n",
					macros[i]->nam, macros[i]->val);
	}

	preprocess();

	free(dirname_pop());

	errno = 0;
	fclose(stdout);
	if(errno)
		die("close():");

	return ret;
usage:
	fprintf(stderr, "Usage: %s [options] files...\n", *argv);
	fputs(" Options:\n"
				"  -Idir: Add search directory\n"
				"  -Dxyz[=abc]: Define xyz (to equal abc)\n"
				"  -Uxyz: Undefine xyz\n"
				"  -o output: output file\n"
				"  -d: increase debug tracing\n"
				"  -L: don't add #line directives\n"
				, stderr);
	return 1;
}
