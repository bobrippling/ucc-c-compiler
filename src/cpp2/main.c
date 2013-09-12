#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <time.h>

#include <sys/stat.h>

#include "../util/util.h"
#include "../util/dynarray.h"
#include "../util/alloc.h"
#include "../util/platform.h"
#include "../util/std.h"

#include "main.h"
#include "macro.h"
#include "preproc.h"
#include "include.h"
#include "directive.h"

#define FNAME_BUILTIN "<builtin>"
#define FNAME_CMDLINE "<command-line>"

static const struct
{
	const char *nam, *val;
} initial_defs[] = {
	/* standard */
	{ "__unix__",       "1"  },
	{ "__STDC__",       "1"  },

#define TYPE(ty, c) { "__" #ty "_TYPE__", #c  }

	TYPE(SIZE, unsigned long),
	TYPE(PTRDIFF, unsigned long),
	TYPE(WINT, unsigned),

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
	{ "__TIMESTAMP__",  NULL },

	{ NULL,             NULL }
};

struct loc loc_tok;
char *current_fname;
char *current_line_str;
int show_current_line = 1;
int no_output = 0;

char cpp_time[16], cpp_date[16], cpp_timestamp[64];

char **cd_stack = NULL;

int option_debug     = 0;
int option_line_info = 1;

void debug_push_line(char *s)
{
	debug_pop_line(); /* currently only a single level */
	current_line_str = ustrdup(s);
}

void debug_pop_line(void)
{
	free(current_line_str), current_line_str = NULL;
}

void dirname_push(char *d)
{
	/*fprintf(stderr, "dirname_push(%s = %p)\n", d, d);*/
	dynarray_add(&cd_stack, d);
}

char *dirname_pop()
{
	return dynarray_pop(char *, &cd_stack);
}

static void calctime(const char *fname)
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

	if(fname){
		struct stat st;
		if(stat(fname, &st) == 0){
			now = localtime(&st.st_mtime);
		}else{
			/* don't touch now - can't open fname,
			 * should get an error later */
		}
	}else{
		/* stdin - don't touch 'now' */
	}

	if(!strftime(cpp_timestamp, sizeof cpp_timestamp,
				"\"%a %b %d %H:%M:%S %Y\"", now))
		die("strftime():");
}


int main(int argc, char **argv)
{
	char *infname, *outfname;
	int ret = 0;
	enum { NONE, MACROS, STATS } dump = NONE;
	int i;
	int platform_win32 = 0;
	int freestanding = 0;
	enum c_std std = STD_C99;

	infname = outfname = NULL;

	current_line = 1;
	current_fname = FNAME_BUILTIN;

	for(i = 0; initial_defs[i].nam; i++)
		macro_add(initial_defs[i].nam, initial_defs[i].val);

	switch(platform_type()){
		case PLATFORM_x86_64:
			macro_add("__LP64__", "1");
			macro_add("__x86_64__", "1");
			/* TODO: __i386__ for 32 bit */
			break;

		case PLATFORM_mipsel_32:
			macro_add("__MIPS__", "1");
	}

	switch(platform_sys()){
#define MAP(t, s) case t: macro_add(s, "1"); break
		MAP(PLATFORM_LINUX,   "__linux__");
		MAP(PLATFORM_FREEBSD, "__FreeBSD__");
#undef MAP

		case PLATFORM_DARWIN:
			macro_add("__DARWIN__", "1");
			macro_add("__MACH__", "1"); /* TODO: proper detection for these */
			macro_add("__APPLE__", "1");
			break;

		case PLATFORM_CYGWIN:
			macro_add("__CYGWIN__", "1");
			platform_win32 = 1;
			break;
	}

	macro_add("__WCHAR_TYPE__",
			platform_win32 ? "short" : "int");

	current_fname = FNAME_CMDLINE;

	for(i = 1; i < argc && *argv[i] == '-'; i++){
		if(!strcmp(argv[i]+1, "-"))
			break;

		switch(argv[i][1]){
			case 'I':
				if(argv[i][2])
					include_add_dir(argv[i]+2);
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

			case 'P':
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
				char *arg = argv[i] + 2;
				char *eq;
				char *directive;

				if(!*arg)
					goto usage;

				/* -D'yo yo' means #define yo yo 1, that is,
				 * we literally generate the #define line */

				eq = strchr(arg, '=');
				if(eq)
					*eq = '\0';

				directive = ustrprintf(
						"define %s %s",
						arg, eq ? eq + 1 : "1");

				parse_internal_directive(directive);
				free(directive);
				break;
			}

			case 'U':
				if(!argv[i][2])
					goto usage;
				macro_remove(argv[i] + 2);
				break;

			case 'd':
				if(argv[i][3])
					goto usage;
				switch(argv[i][2]){
					case 'M':
					case 'S':
						/* list #defines */
						dump = argv[i][2] == 'M' ? MACROS : STATS;
						no_output = 1;
						option_line_info = 0;
						break;
					default:
						goto usage;
				}
				break;

			case '\0':
				/* we've been passed "-" as a filename */
				break;

			case 'f':
				if(!strcmp(argv[i]+2, "freestanding"))
					freestanding = 1;
				else
					goto usage;
				break;

			default:
				if(std_from_str(argv[i], &std) == 0){
					/* we have an std */
				}else{
					goto usage;
				}
		}
	}

	current_fname = FNAME_BUILTIN;

	macro_add("__STDC_HOSTED__",  freestanding ? "0" : "1");
	switch(std){
		case STD_C89:
		case STD_C90:
			/* no */
			break;
		case STD_C99:
			macro_add("__STDC_VERSION__", "199901L");
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

	calctime(infname);

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

	preprocess();

	switch(dump){
		case NONE:
			break;
		case MACROS:
			macros_dump();
			break;
		case STATS:
			macros_stats();
			break;
	}

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
				"  -P: don't add #line directives\n"
				"  -dM: debug output\n"
				"  -dS: print macro usage stats\n"
				"  -MM: generate Makefile dependencies\n"
				, stderr);
	return 1;
}
