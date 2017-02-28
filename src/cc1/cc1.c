#define _POSIX_SOURCE 1 /* fdopen */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <assert.h>

#include <unistd.h>
#include <signal.h>

#include "../util/util.h"
#include "../util/io.h"
#include "../util/platform.h"
#include "../util/math.h"
#include "../util/dynarray.h"
#include "../util/tmpfile.h"
#include "../util/alloc.h"
#include "../util/macros.h"

#include "tokenise.h"
#include "cc1.h"
#include "fold.h"
#include "out/asm.h" /* NUM_SECTIONS */
#include "out/dbg.h" /* dbg_out_filelist() */
#include "gen_asm.h"
#include "gen_dump.h"
#include "gen_style.h"
#include "sym.h"
#include "fold_sym.h"
#include "ops/__builtin.h"
#include "out/asm.h" /* NUM_SECTIONS */
#include "pass1.h"
#include "type_nav.h"
#include "cc1_where.h"
#include "fopt.h"

#include "../config_as.h"

static const char **system_includes;

static struct
{
	char type;
	const char *arg;
	int mask;
} mopts[] = {
	{ 'm',  "stackrealign", MOPT_STACK_REALIGN },
	{ 'm',  "32", MOPT_32 },
	{ 'm',  "64", ~MOPT_32 },
	{ 'm',  "align-is-p2", MOPT_ALIGN_IS_POW2 },

	{ 0,  NULL, 0 }
};

static struct
{
	char pref;
	const char *arg;
	int *pval;
} val_args[] = {
	{ 'f', "error-limit", &cc1_error_limit },
	{ 'f', "message-length", &warning_length },

	{ 'm', "preferred-stack-boundary", &cc1_mstack_align },
	{ 0, NULL, NULL }
};

FILE *cc_out[NUM_SECTIONS];     /* temporary section files */
FILE *cc1_out;                  /* final output */
char *cc1_first_fname;

enum cc1_backend cc1_backend = BACKEND_ASM;

enum mopt mopt_mode = 0;
enum san_opts cc1_sanitize = 0;
char *cc1_sanitize_handler_fn;

enum visibility cc1_visibility_default;

int cc1_mstack_align; /* align stack to n, platform_word_size by default */
int cc1_gdebug;

enum c_std cc1_std = STD_C99;

int cc1_error_limit = 16;

static int caught_sig = 0;

int show_current_line;

struct cc1_fopt cc1_fopt;

struct section sections[NUM_SECTIONS] = {
	{ "text", QUOTE(SECTION_NAME_TEXT) },
	{ "data", QUOTE(SECTION_NAME_DATA) },
	{ "bss",  QUOTE(SECTION_NAME_BSS) },
	{ "rodata", QUOTE(SECTION_NAME_RODATA) },
	{ "ctors", QUOTE(SECTION_NAME_CTORS) },
	{ "dtors", QUOTE(SECTION_NAME_DTORS) },
	{ "dbg_abrv", QUOTE(SECTION_NAME_DBG_ABBREV) },
	{ "dbg_info", QUOTE(SECTION_NAME_DBG_INFO) },
	{ "dbg_line", QUOTE(SECTION_NAME_DBG_LINE) },
};

static FILE *infile;

ucc_printflike(1, 2)
ucc_noreturn
static void ccdie(const char *fmt, ...)
{
	int i = strlen(fmt);
	va_list l;

	va_start(l, fmt);
	vfprintf(stderr, fmt, l);
	va_end(l);

	if(fmt[i-1] == ':'){
		fputc(' ', stderr);
		perror(NULL);
	}else{
		fputc('\n', stderr);
	}

	exit(1);
}

int where_in_sysheader(const where *w)
{
	return w->is_sysh;
}

static void io_cleanup(void)
{
	int i;

	if(caught_sig)
		return;

	for(i = 0; i < NUM_SECTIONS; i++){
		if(!cc_out[i])
			continue;

		if(fclose(cc_out[i]) == EOF)
			fprintf(stderr, "close tmpfile: %s\n", strerror(errno));
	}
}

static void io_setup(void)
{
	int i;

	if(!cc1_out)
		cc1_out = stdout;

	for(i = 0; i < NUM_SECTIONS; i++){
		char *fname;
		int fd = tmpfile_prefix_out("cc1_", &fname);

		if(fd < 0)
			ccdie("tmpfile(%s):", fname);

		if(remove(fname) != 0)
			fprintf(stderr, "remove %s: %s\n", fname, strerror(errno));

		cc_out[i] = fdopen(fd, "w+"); /* need to seek */
		assert(cc_out[i]);

		free(fname);
	}

	atexit(io_cleanup);
}

static int should_emit_gnu_stack_note(void)
{
	return platform_sys() == PLATFORM_LINUX;
}

static void io_fin(int do_sections, const char *fname)
{
	const int execstack = 0;
	int i;

	(void)fname;

	for(i = 0; i < NUM_SECTIONS; i++){
		/* cat cc_out[i] to cc1_out, with section headers */
		int emit_this_section = 1;

		if(cc1_gdebug && (i == SECTION_TEXT || i == SECTION_DBG_LINE)){
			/* need .text for debug to reference */
		}else if(asm_section_empty(i)){
			emit_this_section = 0;
		}

		if(do_sections && emit_this_section){
			char buf[256];
			long last = ftell(cc_out[i]);

			if(last == -1 || fseek(cc_out[i], 0, SEEK_SET) == -1)
				ccdie("seeking on section file %d:", i);

			if(fprintf(cc1_out, ".section %s\n", sections[i].name) < 0
			|| fprintf(cc1_out, "%s%s:\n", SECTION_BEGIN, sections[i].desc) < 0)
			{
				ccdie("write to cc1 output:");
			}

			while(fgets(buf, sizeof buf, cc_out[i]))
				if(fputs(buf, cc1_out) == EOF)
					ccdie("write to cc1 output:");

			if(ferror(cc_out[i]))
				ccdie("read from section file %d:", i);

			if(fprintf(cc1_out, "%s%s:\n", SECTION_END, sections[i].desc) < 0)
				ccdie("terminating section %d:", i);
		}
	}

	if(should_emit_gnu_stack_note()
	&& fprintf(cc1_out,
			".section .note.GNU-stack,\"%s\",@progbits\n",
			execstack ? "x" : "") < 0)
	{
		ccdie(0, "write to cc1 output:");
	}

	if(fclose(cc1_out))
		ccdie("close cc1 output");
}

static void sigh(int sig)
{
	(void)sig;
	caught_sig = 1;
	io_cleanup();
}

static char *next_line(void)
{
	char *s = fline(infile, NULL);
	char *p;

	if(!s){
		if(feof(infile))
			return NULL;
		else
			die("read():");
	}

	for(p = s; *p; p++)
		if(*p == '\r')
			*p = ' ';

	return s;
}

static void gen_backend(symtable_global *globs, const char *fname)
{
	void (*gf)(symtable_global *) = NULL;

	switch(cc1_backend){
		case BACKEND_STYLE:
			gf = gen_style;
			if(0){
		case BACKEND_DUMP:
				gf = gen_dump;
			}
			gf(globs);
			break;

		case BACKEND_ASM:
		{
			char buf[4096];
			char *compdir;
			struct out_dbg_filelist *filelist;

			compdir = getcwd(NULL, 0);
			if(!compdir){
				/* no auto-malloc */
				compdir = getcwd(buf, sizeof(buf)-1);
				/* PATH_MAX may not include the  ^ nul byte */
				if(!compdir)
					die("getcwd():");
			}

			gen_asm(globs,
					cc1_first_fname ? cc1_first_fname : fname,
					compdir,
					&filelist);

			/* filelist needs to be output first */
			if(filelist && cc1_gdebug)
				dbg_out_filelist(filelist, cc1_out);


			if(compdir != buf)
				free(compdir);

			io_fin(gf == NULL, fname);
			break;
		}
	}
}

ucc_printflike(2, 3)
ucc_noreturn
static void usage(const char *argv0, const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	vfprintf(stderr, fmt, l);
	va_end(l);

	ccdie(
			"Usage: %s [-W[no-]warning] [-f[no-]option] [-m[no-]machine] [-o output] file",
			argv0);
}

static int optimise(const char *argv0, const char *arg)
{
	/* TODO: -fdce, -fthread-jumps, -falign-{functions,jumps,loops,labels}
	 * -fdelete-null-pointer-checks, -freorder-blocks
	 */
	enum { O0, O1, O2, O3, Os } opt = O0;

	if(!*arg){
		/* -O means -O2 */
		opt = O2;
	}else if(arg[1]){
		goto unrecog;
	}else switch(arg[0]){
		default:
			goto unrecog;

		case '0': opt = O0; break;
		case '1': opt = O1; break;
		case '2': opt = O2; break;
		case '3': opt = O3; break;
		case 's': opt = Os; break;
	}

	switch(opt){
		case O0:
			cc1_fopt.thread_jumps = 0;
			break;

		case Os:
			/* same as -O2 but disable inlining and int-float-load */
			cc1_fopt.fold_const_vlas = 1;
			cc1_fopt.inline_functions = 0;
			cc1_fopt.integral_float_load = 0;
			break;

		case O1:
		case O2:
		case O3:
			cc1_fopt.fold_const_vlas = 1;
			cc1_fopt.inline_functions = 1;
			cc1_fopt.integral_float_load = 1;
			cc1_fopt.thread_jumps = 1;
			break;
	}

	return 0;
unrecog:
	fprintf(stderr, "%s: unrecognised optimisation flag -O%c\n", argv0, arg[0]);
	return 1;
}

static void add_sanitize_option(const char *argv0, const char *san)
{
	if(!strcmp(san, "undefined")){
		cc1_sanitize |= CC1_UBSAN;
		cc1_fopt.trapv = 1;
	}else{
		fprintf(stderr, "%s: unknown sanitize option '%s'\n", argv0, san);
		exit(1);
	}
}

static void set_sanitize_error(const char *argv0, const char *handler)
{
	free(cc1_sanitize_handler_fn);
	cc1_sanitize_handler_fn = NULL;

	if(!strcmp(handler, "trap")){
		/* fine */
	}else if(!strncmp(handler, "call=", 5)){
		cc1_sanitize_handler_fn = ustrdup(handler + 5);

		if(!*cc1_sanitize_handler_fn){
			fprintf(stderr, "%s: empty sanitize function handler\n", argv0);
			exit(1);
		}

	}else{
		fprintf(stderr, "%s: unknown sanitize handler '%s'\n", argv0, handler);
		exit(1);
	}
}

static void set_default_visibility(const char *argv0, const char *visibility)
{
	if(!visibility_parse(&cc1_visibility_default, visibility)){
		fprintf(stderr, "%s: unknown/unsupported visibility \"%s\"\n", argv0, visibility);
		exit(1);
	}
}

static int parse_mf_equals(
		const char *argv0,
		char arg_ty,
		const char *arg_substr,
		char *equal,
		int invert)
{
	int found = 0;
	int i;
	int new_val;

	if(invert){
		usage(argv0, "\"no-\" unexpected for value-argument\n");
	}

	if(!strncmp(arg_substr, "sanitize=", 9)){
		add_sanitize_option(argv0, arg_substr + 9);
		return 1;
	}else if(!strncmp(arg_substr, "sanitize-error=", 15)){
		set_sanitize_error(argv0, arg_substr + 15);
		return 1;
	}else if(!strncmp(arg_substr, "visibility=", 11)){
		set_default_visibility(argv0, arg_substr + 11);
		return 1;
	}

	if(sscanf(equal + 1, "%d", &new_val) != 1){
		usage(argv0, "need number for %s\n", arg_substr);
	}

	*equal = '\0';
	for(i = 0; val_args[i].arg; i++){
		if(val_args[i].pref == arg_ty && !strcmp(arg_substr, val_args[i].arg)){
			*val_args[i].pval = new_val;
			found = 1;
			break;
		}
	}

	return found;
}

static int mopt_on(const char *argument, int invert)
{
	int i;
	for(i = 0; mopts[i].arg; i++){
		if(!strcmp(argument, mopts[i].arg)){
			/* if the mask isn't a single bit, treat it as an unmask */
			const int unmask = mopts[i].mask & (mopts[i].mask - 1);

			if(invert){
				if(unmask)
					mopt_mode |= ~mopts[i].mask;
				else
					mopt_mode &= ~mopts[i].mask;
			}else{
				if(unmask)
					mopt_mode &= mopts[i].mask;
				else
					mopt_mode |= mopts[i].mask;
			}
			return 1;
		}
	}
	return 0;
}

static void parse_Wmf_option(
		const char *argv0,
		char *argument,
		int *const werror,
		dynmap *unknown_warnings)
{
	const char arg_ty = argument[1];
	const char *arg_substr = argument + 2;
	int invert = 0;
	char *equal;

	if(!strncmp(arg_substr, "no-", 3)){
		arg_substr += 3;
		invert = 1;
	}

	/* -f and -m may accept values. -W doesn't, so check that first */
	if(arg_ty == 'W'){
		warning_on(arg_substr, invert ? W_OFF : W_WARN, werror, unknown_warnings);
		return;
	}

	equal = strchr(argument, '=');
	if(equal){
		if(!parse_mf_equals(argv0, arg_ty, arg_substr, equal, invert))
			goto unknown;
		return;
	}

	if(arg_ty == 'f'){
		if(fopt_on(&cc1_fopt, arg_substr, invert))
			return;
		goto unknown;
	}

	if(arg_ty == 'm'){
		if(mopt_on(arg_substr, invert))
			return;
		goto unknown;
	}

unknown:
	usage(argv0, "unrecognised warning/feature/machine option \"%s\"\n", argument);
}

int main(int argc, char **argv)
{
	int failure;
	where loc_start;
	static symtable_global *globs;
	const char *fname;
	int i;
	int werror = 0;
	dynmap *unknown_warnings = dynmap_new(char *, strcmp, dynmap_strhash);

	/*signal(SIGINT , sigh);*/
	signal(SIGQUIT, sigh);
	signal(SIGTERM, sigh);
	signal(SIGABRT, sigh);
	signal(SIGSEGV, sigh);

	fname = NULL;

	/* defaults */
	cc1_mstack_align = log2i(platform_word_size());
	warning_init();
	fopt_default(&cc1_fopt);

	for(i = 1; i < argc; i++){
		if(!strncmp(argv[i], "-emit", 5)){
			const char *emit;

			switch(argv[i][5]){
				case '=':
					emit = argv[i] + 6;
					break;

				case '\0':
					if(++i == argc)
						usage(argv[0], "-emit needs an argument");
					emit = argv[i];
					break;

				default:
					usage(argv[0], "unrecognised argument \"%s\" (did you mean -emit=...?)\n", argv[i]);
			}


			if(!strcmp(emit, "dump") || !strcmp(emit, "print"))
				cc1_backend = BACKEND_DUMP;
			else if(!strcmp(emit, "asm"))
				cc1_backend = BACKEND_ASM;
			else if(!strcmp(emit, "style"))
				cc1_backend = BACKEND_STYLE;
			else
				usage(argv[0], "unknown emit backend \"%s\"", emit);

		}else if(!strncmp(argv[i], "-g", 2)){
			switch(argv[i][2]){
				case '0':
					if(argv[i][3]){
				default:
						die("-g extra argument unexpected");
					}
					cc1_gdebug = 0;
					break;
				case '\0':
					cc1_gdebug = 1;
					break;
			}

		}else if(!strcmp(argv[i], "-o")){
			if(++i == argc)
				usage(argv[0], "-o needs an argument");

			if(strcmp(argv[i], "-")){
				cc1_out = fopen(argv[i], "w");
				if(!cc1_out){
					ccdie("open %s:", argv[i]);
					goto out;
				}
			}

		}else if(!strncmp(argv[i], "-std=", 5) || !strcmp(argv[i], "-ansi")){
			int gnu;

			if(std_from_str(argv[i], &cc1_std, &gnu))
				ccdie("-std argument \"%s\" not recognised", argv[i]);

			if(gnu)
				cc1_fopt.ext_keywords = 1;
			else
				cc1_fopt.ext_keywords = 0;

		}else if(!strcmp(argv[i], "-w")){
			warnings_set(W_OFF);

		}else if(!strcmp(argv[i], "-pedantic") || !strcmp(argv[i], "-pedantic-errors")){
			const int errors = (argv[i][9] != '\0');

			warning_pedantic(errors ? W_ERROR : W_WARN);

		}else if(!strcmp(argv[i], "-pedantic-errors")){
			warning_pedantic(W_ERROR);

		}else if(argv[i][0] == '-'
		&& (argv[i][1] == 'W' || argv[i][1] == 'f' || argv[i][1] == 'm')){
			parse_Wmf_option(*argv, argv[i], &werror, unknown_warnings);

		}else if(!strncmp(argv[i], "-O", 2)){
			if(optimise(*argv, argv[i] + 2))
				exit(1);

		}else if(!fname){
			fname = argv[i];
		}else{
			usage(argv[0], "unknown argument %s", argv[i]);
		}
	}

	/* sanity checks */
	{
		const unsigned new = powf(2, cc1_mstack_align);
		if(new < platform_word_size())
			ccdie("stack alignment must be >= platform word size (2^%d)",
					log2i(platform_word_size()));

		cc1_mstack_align = new;
	}

	if(werror)
		warnings_upgrade();

	if(warnings_check_unknown(unknown_warnings)){
		failure = 1;
		goto out;
	}

	if(cc1_fopt.pie){
		/* -fpie/PIE implies -fpic/PIC and __attribute__((visibility("protected"))) for all symbols */
		cc1_fopt.pic = 1;
		if(AS_SUPPORTS_VISIBILITY_PROTECTED)
			cc1_visibility_default = VISIBILITY_PROTECTED;
	}


	if(fname && strcmp(fname, "-")){
		infile = fopen(fname, "r");
		if(!infile)
			ccdie("open %s:", fname);
	}else{
		infile = stdin;
		fname = "-";
	}

	io_setup();

	show_current_line = cc1_fopt.show_line;

	cc1_type_nav = type_nav_init();

	tokenise_set_mode(
			(cc1_fopt.ext_keywords ? KW_EXT : 0) |
			(cc1_std >= STD_C99 ? KW_C99 : 0));

	tokenise_set_input(next_line, fname);

	where_cc1_current(&loc_start);
	globs = symtabg_new(&loc_start);

	failure = parse_and_fold(globs);

	if(infile != stdin)
		fclose(infile), infile = NULL;

	if(failure == 0 || /* attempt dump anyway */cc1_backend == BACKEND_DUMP){
		gen_backend(globs, fname);
		if(gen_had_error)
			failure = 1;
	}

	if(cc1_fopt.dump_type_tree)
		type_nav_dump(cc1_type_nav);

out:
	dynarray_free(const char **, system_includes, NULL);
	{
		size_t i;
		char *key;
		for(i = 0; (key = dynmap_key(char *, unknown_warnings, i)); i++)
			free(key);
		dynmap_free(unknown_warnings);
	}

	return failure;
}
