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
#include "../util/limits.h"
#include "../util/macros.h"

#include "main.h"
#include "macro.h"
#include "preproc.h"
#include "include.h"
#include "directive.h"
#include "deps.h"
#include "feat.h"
#include "str.h"

static const struct
{
	const char *nam, *val;
	int is_fn;

} initial_defs[] = {
	/* standard */
	{ "__unix__",       "1", 0 },
	{ "__STDC__",       "1", 0 },

#if !UCC_HAS_ATOMICS
	{ "__STDC_NO_ATOMICS__" , "1", 0 }, /* _Atomic */
#endif
#if !UCC_HAS_THREADS
	{ "__STDC_NO_THREADS__" , "1", 0 }, /* _Thread_local */
#endif
#if !UCC_HAS_COMPLEX
	{ "__STDC_NO_COMPLEX__", "1", 0 }, /* _Complex */
#endif
#if !UCC_HAS_VLA
	{ "__STDC_NO_VLA__", "1", 0 },
#endif

#define TYPE(ty, c) { "__" #ty "_TYPE__", #c, 0 }

	TYPE(SIZE, unsigned long),
	TYPE(PTRDIFF, unsigned long),
	TYPE(WINT, unsigned),

	{ "__ORDER_LITTLE_ENDIAN__", "1234", 0 },
	{ "__ORDER_BIG_ENDIAN__",    "4321", 0 },
	{ "__ORDER_PDP_ENDIAN__",    "3412", 0 },

	/* non-standard */
	{ "__BLOCKS__",     "1", 0 },

	/* custom */
	{ "__UCC__",        "1", 0 },

	/* magic */
#define SPECIAL(x) { x, NULL, 0 }
	SPECIAL("__FILE__"),
	SPECIAL("__LINE__"),
	SPECIAL("__COUNTER__"),
	SPECIAL("__DATE__"),
	SPECIAL("__TIME__"),
	SPECIAL("__TIMESTAMP__"),
	SPECIAL("__BASE_FILE__"),

#undef SPECIAL
#define SPECIAL(x) { x, NULL, 1 }
	SPECIAL("__has_feature"),
	SPECIAL("__has_extension"),
	SPECIAL("__has_attribute"),
	SPECIAL("__has_builtin"),

	/* here for defined(__has_include), then special cased to prevent expansion outside of #if */
	SPECIAL("__has_include"),
#undef SPECIAL

	{ NULL, NULL, 0 }
};

struct loc loc_tok;
char *current_fname;
int current_fname_used;
char *current_line_str;
int show_current_line = 1;
int no_output = 0;
int missing_header_error = 1;

char cpp_time[16], cpp_date[16], cpp_timestamp[64];
char *cpp_basefile;

char **cd_stack = NULL;

int option_line_info = 1;
int option_trigraphs = 0, option_digraphs = 0;
static int option_trace = 0;

enum c_std cpp_std = STD_C99;

enum wmode wmode =
	  WWHITESPACE
	| WTRAILING
	| WEMPTY_ARG
	| WPASTE
	| WFINALESCAPE
	| WMULTICHAR
	| WQUOTE
	| WHASHWARNING
	| WBACKSLASH_SPACE_NEWLINE;

enum comment_strip strip_comments = STRIP_ALL;
int option_show_include_nesting;

static const struct
{
	const char *warn, *desc;
	enum wmode or_mask;
} warns[] = {

#define X(arg, desc, flag) { arg, desc, flag },
#include "warnings.def"
#undef X
};

#define ITER_WARNS(j) for(j = 0; j < countof(warns); j++)

void trace(const char *fmt, ...)
{
	va_list l;
	if(!option_trace)
		return;

	va_start(l, fmt);
	vfprintf(stderr, fmt, l);
	va_end(l);
}

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

char *dirname_pop(void)
{
	return dynarray_pop(char *, &cd_stack);
}

void cpp_where_current(where *w)
{
  where_current(w);
  current_fname_used = 1;
}

void set_current_fname(const char *new)
{
	if(current_fname == new)
		return;
	if(!current_fname_used)
		free(current_fname);
	current_fname = ustrdup(new);
	current_fname_used = 0;
}

static struct tm *current_time(int *const using_env)
{
	const char *source_date_epoch = getenv("SOURCE_DATE_EPOCH");
	struct tm *build_time;
	time_t t;

	*using_env = !!source_date_epoch;

	if (source_date_epoch) {
		unsigned long epoch;
		char *end;

		errno = 0;
		epoch = strtoul(source_date_epoch, &end, 10);

		if(errno)
			die("couldn't parse $SOURCE_DATE_EPOCH:");

		if(*end)
			die("$SOURCE_DATE_EPOCH isn't a number");

		t = epoch;
	}else{
		t = time(NULL);
	}

	build_time = gmtime(&t);
	if(!build_time)
		die("gmtime():");
	return build_time;
}

static void calctime(const char *fname)
{
	int using_env;
	struct tm *now = current_time(&using_env);

#define FTIME(s, fmt) \
	if(!strftime(s, sizeof s, fmt, now)) \
		die("strftime():");

	FTIME(cpp_time, "\"%H:%M:%S\"");
	FTIME(cpp_date, "\"%b %d %G\"");

	if(fname && !using_env){
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

static void macro_add_limits(void)
{
#define QUOTE_(x) #x
#define QUOTE(x) QUOTE_(x)
#define MACRO_ADD_LIM(m) macro_add("__" #m "__", QUOTE(__ ## m ## __), 0)
	MACRO_ADD_LIM(SCHAR_MAX);
	MACRO_ADD_LIM(SHRT_MAX);
	MACRO_ADD_LIM(INT_MAX);
	MACRO_ADD_LIM(LONG_MAX);
	MACRO_ADD_LIM(LONG_LONG_MAX);
#undef MACRO_ADD_LIM
#undef QUOTE
#undef QUOTE_
}

static void add_platform_dependant_macros(void)
{
	int platform_win32 = 0;
	if(platform_bigendian())
		macro_add("__BYTE_ORDER__", "__ORDER_BIG_ENDIAN__", 0);
	else
		macro_add("__BYTE_ORDER__", "__ORDER_LITTLE_ENDIAN__", 0);

	switch(platform_sys()){
		case SYS_linux:
			macro_add("__linux__", "1", 0);
			break;

		case SYS_darwin:
			macro_add("__DARWIN__", "1", 0);
			macro_add("__MACH__", "1", 0); /* TODO: proper detection for these */
			macro_add("__APPLE__", "1", 0);
			break;

		case SYS_cygwin:
			macro_add("__CYGWIN__", "1", 0);
			platform_win32 = 1;
			break;

		case SYS_freebsd:
			break;
	}

	macro_add("__WCHAR_TYPE__",
			platform_win32 ? "short" : "int", 0);

	macro_add_sprintf("__BIGGEST_ALIGNMENT__", "%u", platform_align_max());

	switch(platform_type()){
		case ARCH_x86_64:
			macro_add("__LP64__", "1", 0);
			macro_add("__x86_64__", "1", 0);
			break;

		case ARCH_i386:
			macro_add("__i386__", "1", 0);
			break;

		/*case PLATFORM_mipsel_32:
			macro_add("__MIPS__", "1", 0);*/
	}
}

static int init_target(const char *target)
{
	struct triple triple;

	if(target){
		const char *bad;
		if(!triple_parse(target, &triple, &bad)){
			fprintf(stderr, "Couldn't parse triple: %s\n", bad);
			return 0;
		}
	}else{
		const char *unparsed;
		if(!triple_default(&triple, &unparsed)){
			fprintf(stderr, "couldn't get target triple: %s\n",
					unparsed ? unparsed : strerror(errno));
			return 0;
		}
	}

	platform_init(triple.arch, triple.sys);
	return 1;
}

int main(int argc, char **argv)
{
	char *infname, *outfname, *depfname;
	int ret = 0;
	enum {
		PREPROCESSED = 1 << 0,
		MACROS = 1 << 1,
		MACROS_WHERE = 1 << 2,
		STATS = 1 << 3,
		DEPS = 1 << 4
	} emit = PREPROCESSED;
	int i;
	int freestanding = 0;
	int offsetof_macro = 0;
	const char *target = NULL;

	infname = outfname = depfname = NULL;

	current_line = 1;
	set_current_fname(FNAME_BUILTIN);

	macro_add_limits();

	for(i = 0; initial_defs[i].nam; i++){
		if(initial_defs[i].is_fn)
			macro_add_func(initial_defs[i].nam, initial_defs[i].val, NULL, 0, 1);
		else
			macro_add(initial_defs[i].nam, initial_defs[i].val, 0);
	}

	set_current_fname(FNAME_CMDLINE);

	for(i = 1; i < argc; i++){
		if(argv[i][0] != '-' || !strcmp(argv[i]+1, "-")){
			if(!infname)
				infname = argv[i];
			else if(!outfname)
				outfname = argv[i];
			else
				goto usage;

			continue;
		}

		switch(argv[i][1]){
			case 'I':
				if(argv[i][2])
					include_add_dir(argv[i]+2, 0);
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

			case 'H':
				option_show_include_nesting = 1;
				break;

			case 'C':
				if(argv[i][2] == '\0')
					strip_comments = STRIP_EXCEPT_DIRECTIVE;
				else if(!strcmp(argv[i] + 2, "C"))
					strip_comments = STRIP_NONE;
				break;

			case 'M':
				if(!strcmp(argv[i] + 2, "M")){
					emit |= DEPS;
					emit &= ~PREPROCESSED;
				}else if(!strcmp(argv[i] + 2, "G")){
					missing_header_error = 0;
				}else if(!strcmp(argv[i] + 2, "D")){
					emit |= DEPS;
				}else if(!strcmp(argv[i] + 2, "F")){
					depfname = argv[i + 1];
					if(!depfname)
						goto usage;
				}else{
					goto usage;
				}
				break;

			case 'D':
			{
				char *arg = argv[i] + 2;
				char *eq;
				char *directive;

				if(!*arg){
					/* allow "-D" "arg" */
					arg = argv[++i];
					if(!arg)
						goto usage;
				}

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
				if(argv[i][2] && argv[i][3])
					goto defaul;
				switch(argv[i][2]){
					case 'M':
					case 'S':
					case 'W':
						/* list #defines */
						emit |= (
								argv[i][2] == 'M' ? MACROS :
								argv[i][2] == 'S' ? STATS :
								MACROS_WHERE);

						emit &= ~PREPROCESSED;
						break;
					case '\0':
						option_trace = 1;
						break;
					default:
						goto usage;
				}
				break;

			case '\0':
				/* we've been passed "-" as a filename */
				break;

			case 'f':
				if(!strcmp(argv[i]+2, "freestanding")){
					freestanding = 1;
				}else if(!strncmp(argv[i]+2, "message-length=", 15)){
					const char *p = argv[i] + 17;
					warning_length = atoi(p);
				}else if(!strcmp(argv[i]+2, "cpp-offsetof")){
					offsetof_macro = 1;
				}else{
					goto usage;
				}
				break;

			case 'W':
			{
				int off;
				unsigned j;
				char *p = argv[i] + 2;

				off = !strncmp(p, "no-", 3);
				if(off)
					p += 3;


				ITER_WARNS(j){
					if(!strcmp(p, warns[j].warn)){
						if(off)
							wmode &= ~warns[j].or_mask;
						else
							wmode |= warns[j].or_mask;
						break;
					}
				}

				/* if not found, we ignore - it was intended for cc1 */
				break;
			}

			case 'O':
			{
				switch(argv[i][2]){
					case '0':
						macro_remove("__OPTIMIZE_SIZE__");
						macro_remove("__OPTIMIZE__");
						break;
					case 's':
						macro_add("__OPTIMIZE_SIZE__",  "1", 0);
						/* fallthru */
					default:
						macro_add("__OPTIMIZE__",  "1", 0);
				}
				break;
			}

			case 'w':
				if(!argv[i][2]){
					wmode = 0;
					break;
				}
				/* fall */


			default:
defaul:
				if(!strcmp(argv[i], "-isystem")){
					if(++i == argc){
						fprintf(stderr, "-isystem needs a parameter");
						goto usage;
					}
					include_add_dir(argv[i], 1);
				}else if(std_from_str(argv[i], &cpp_std, NULL) == 0){
					/* we have an std */
				}else if(!strcmp(argv[i], "-trigraphs")){
					option_trigraphs = 1;
				}else if(!strcmp(argv[i], "-digraphs")){
					option_digraphs = 1;
				}else if(!strcmp(argv[i], "-target")){
					i++;
					if(!argv[i]){
						fprintf(stderr, "-target requires an argument\n");
						goto usage;
					}
					target = argv[i];
				}else{
					fprintf(stderr, "unrecognised option \"%s\"\n", argv[i]);
					goto usage;
				}
		}
	}

	no_output = !(emit & PREPROCESSED);

	if(!missing_header_error && !(emit & DEPS)){
		fprintf(stderr, "%s: -MG requires -MM\n", *argv);
		return 1;
	}

	if(!init_target(target))
		return 1;

	add_platform_dependant_macros();

	set_current_fname(FNAME_BUILTIN);

	macro_add("__STDC_HOSTED__",  freestanding ? "0" : "1", 0);
	switch(cpp_std){
		case STD_C89:
		case STD_C90:
			/* no */
			break;
		case STD_C99:
			macro_add("__STDC_VERSION__", "199901L", 0);
			break;
		case STD_C11:
			macro_add("__STDC_VERSION__", "201112L", 0);
			break;
		case STD_C18:
			macro_add("__STDC_VERSION__", "201710L", 0);
			break;
		case STD_C2X:
			macro_add("__STDC_VERSION__", "202000L", 0);
			break;
	}

	if(offsetof_macro){
		char **args = umalloc(3 * sizeof *args);

		args[0] = ustrdup("T");
		args[1] = ustrdup("memb");

		macro_add_func("__builtin_offsetof",
				"(unsigned long)&((T *)0)->memb",
				args, 0, 0);
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

	set_current_fname(infname);
	cpp_basefile = str_quote(infname, 0);

	preprocess();

	if(wmode & WUNUSED)
		macros_warn_unused();

	if(emit & (MACROS | MACROS_WHERE))
		macros_dump(emit == MACROS_WHERE);
	if(emit & STATS)
		macros_stats();
	if(emit & DEPS)
		deps_dump(infname, depfname);

	free(dirname_pop());
	free(cpp_basefile);

	errno = 0;
	fclose(stdout);
	if(errno)
		die("close():");

	return ret;
usage:
	fprintf(stderr, "Usage: %s [options] in-file out-file\n", *argv);
	fputs(" Options:\n"
				"  -Idir: Add search directory\n"
				"  -isystem dir: Add system search directory\n"
				"  -Dxyz[=abc]: Define xyz (to equal abc)\n"
				"  -Uxyz: Undefine xyz\n"
				"  -o output: output file\n"
				"  -P: don't add #line directives\n"
				"  -trigraphs: enable trigraphs\n"
				"  -digraphs: enable digraphs\n"
				"  -w: disable all warnings\n"
				"\n"
				"  -MM: generate Makefile dependencies\n"
				"  -MG: ignore missing headers, count as dependency\n"
				"  -MD: emit dependencies on standard out\n"
				"  -MF: (with -MD) emit dependencies to given file\n"
				"\n"
				"  -f[no-]freestanding: control __STDC_HOSTED__\n"
				"  -std=[standard]: control __STDC_VERSION__\n"
				"  -fmessage-length=...: control warning message length\n"
				"  -f[no-]cpp-offsetof: define __builtin_offsetof as a macro\n"
				"\n"
				"  -C: don't discard comments, except in macros\n"
				"  -CC: don't discard comments, even in macros\n"
				"\n"
				"  -m32/-m64: control architecture specific definitions\n"
				"  -O[opt]: control optimisation macro definitions\n"
				"\n"
				"  -dM: output macro debugging information\n"
				"  -dS: output stats debugging information\n"
				"  -dW: output macro location debugging information\n"
				"  -d: output trace debugging information\n"
				"  -H: show header includes and nesting depth\n"
				"\n"
				, stderr);

	{
		unsigned i;
		ITER_WARNS(i)
			fprintf(stderr, "  -W%s: %s\n", warns[i].warn, warns[i].desc);
	}

	return 1;
}
