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
#include "cc1_target.h"
#include "cc1_out.h"
#include "cc1_sections.h"

static const char **system_includes;

struct version
{
	int maj, min;
};

static struct
{
	char type;
	const char *arg;
	int mask;
} mopts[] = {
	{ 'm',  "stackrealign", MOPT_STACK_REALIGN },
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

dynmap *cc1_out_persection; /* char* => FILE* */
enum section_builtin cc1_current_section = -1;
FILE *cc1_current_section_file;
char *cc1_first_fname;

enum cc1_backend cc1_backend = BACKEND_ASM;

enum mopt mopt_mode = 0;
enum san_opts cc1_sanitize = 0;
char *cc1_sanitize_handler_fn;

enum visibility cc1_visibility_default;

int cc1_mstack_align; /* align stack to n, platform_word_size by default */
enum debug_level cc1_gdebug = DEBUG_OFF;
int cc1_gdebug_columninfo = 1;

enum c_std cc1_std = STD_C99;

int cc1_error_limit = 16;

int show_current_line;

struct cc1_fopt cc1_fopt;

struct target_details cc1_target_details;

static const char *debug_compilation_dir;

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

static void dump_options(void)
{
	int i;

	fprintf(stderr, "Output options\n");
	fprintf(stderr, "  -g0, -gline-tables-only, -g[no-]column-info\n");
	fprintf(stderr, "  -o output-file\n");
	fprintf(stderr, "  -emit=(dump|print|asm|style)\n");
	fprintf(stderr, "  -m(stringop-strategy=...|...)\n");
	fprintf(stderr, "  -O[0123s]\n");
	fprintf(stderr, "  --help\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Input options\n");
	fprintf(stderr, "  -pedantic{,-errors}\n");
	fprintf(stderr, "  -W(no-)?(all|extra|everything|gnu|error(=...)|...)\n");
	fprintf(stderr, "  -w\n");
	fprintf(stderr, "  -std=(gnu|c)(99|90|89|11), -ansi\n");
	fprintf(stderr, "  -f(sanitize=...|sanitize-error=...)\n");
	fprintf(stderr, "  -fvisibility=default|hidden|protected\n");
	fprintf(stderr, "  -fdebug-compilation-dir=...\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Feature options\n");

#define X(flag, memb) fprintf(stderr, "  -f[no-]" flag "\n");
#define ALIAS X
#define INVERT X
#define EXCLUSIVE(flag, name, excl) X(flag, name)
#define ALIAS_EXCLUSIVE(flag, name, excl) X(flag, name)
#include "fopts.h"
#undef X
#undef ALIAS
#undef INVERT
#undef EXCLUSIVE
#undef ALIAS_EXCLUSIVE

	fprintf(stderr, "\n");
	fprintf(stderr, "Machine options\n");
	for(i = 0; mopts[i].arg; i++)
		fprintf(stderr, "  -m[no-]%s\n", mopts[i].arg);

	fprintf(stderr, "\n");
	fprintf(stderr, "Feature/machine values\n");
	for(i = 0; val_args[i].arg; i++)
		fprintf(stderr, "  -%c%s=value\n", val_args[i].pref, val_args[i].arg);
}

int where_in_sysheader(const where *w)
{
	return w->is_sysh;
}

static int should_emit_gnu_stack_note(void)
{
	return platform_sys() == SYS_linux;
}

static int should_emit_macosx_version_min(struct version *const min)
{
	if(platform_sys() != SYS_darwin)
		return 0;

	min->maj = 10;
	min->min = 5;
	return 1;
}

static void io_fin_gnustack(FILE *out)
{
	const int execstack = 0;

	if(should_emit_gnu_stack_note()
	&& fprintf(out,
			".section .note.GNU-stack,\"%s\",@progbits\n",
			execstack ? "x" : "") < 0)
	{
		ccdie("write to cc1 output:");
	}
}

static void io_fin_macosx_version(FILE *out)
{
	struct version macosx_version_min;
	if(should_emit_macosx_version_min(&macosx_version_min)
	&& fprintf(out,
		".macosx_version_min %d, %d\n",
		macosx_version_min.maj,
		macosx_version_min.min) < 0)
	{
		ccdie("write to cc1 output:");
	}
}

static void io_fin_section(FILE *section, FILE *out, const char *name)
{
	enum section_builtin sec = asm_builtin_section_from_str(name);
	const char *desc = NULL;

	if((int)sec != -1)
		desc = asm_section_desc(sec);

	if(fseek(section, 0, SEEK_SET))
		ccdie("seeking in section tmpfile:");

	xfprintf(out, ".section %s\n", name);

	if(desc)
		xfprintf(out, "%s%s%s:\n", cc1_target_details.as.privatelbl_prefix, SECTION_BEGIN, desc);

	if(cat(section, out))
		ccdie("concatenating section tmpfile:");

	if(desc)
		xfprintf(out, "%s%s%s:\n", cc1_target_details.as.privatelbl_prefix, SECTION_END, desc);

	if(fclose(section))
		ccdie("closing section tmpfile:");
}

static void io_fin_sections(FILE *out)
{
	FILE *section;
	size_t i;

	if(!cc1_out_persection)
		return;

	if(cc1_gdebug){
		/* ensure we have text and debug-line sections for the debug to reference */
		(void)asm_section_file(SECTION_TEXT);
		(void)asm_section_file(SECTION_DBG_LINE);
	}

	for(i = 0; (section = dynmap_value(FILE *, cc1_out_persection, i)); i++){
		char *name = dynmap_key(char *, cc1_out_persection, i);

		io_fin_section(section, out, name);
		free(name);
	}

	dynmap_free(cc1_out_persection);
}

static void io_fin(FILE *out)
{
	io_fin_sections(out);
	io_fin_gnustack(out);
	io_fin_macosx_version(out);
}

static char *next_line(void)
{
	char *s = fline(infile, NULL);
	char *p;

	if(!s){
		if(feof(infile))
			return NULL;
		else
			ccdie("read():");
	}

	for(p = s; *p; p++)
		if(*p == '\r')
			*p = ' ';

	return s;
}

static void gen_backend(symtable_global *globs, const char *fname, FILE *out)
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

			compdir = (char *)debug_compilation_dir;
			if(!compdir)
				compdir = getcwd(NULL, 0);
			if(!compdir){
				/* no auto-malloc */
				compdir = getcwd(buf, sizeof(buf)-1);
				/* PATH_MAX may not include the  ^ nul byte */
				if(!compdir)
					ccdie("getcwd():");
			}

			gen_asm(globs,
					cc1_first_fname ? cc1_first_fname : fname,
					compdir,
					&filelist);

			/* filelist needs to be output first */
			if(filelist && cc1_gdebug != DEBUG_OFF)
				dbg_out_filelist(filelist, out);

			io_fin(out);

			if(compdir != buf && compdir != debug_compilation_dir)
				free(compdir);
			break;
		}
	}
}

ucc_printflike(2, 3)
ucc_noreturn
static void usage(const char *argv0, const char *fmt, ...)
{
	if(fmt){
		va_list l;
		va_start(l, fmt);
		vfprintf(stderr, fmt, l);
		va_end(l);
	}

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
	if(!visibility_parse(&cc1_visibility_default, visibility, cc1_target_details.as.supports_visibility_protected)){
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
	}else if(!strncmp(arg_substr, "debug-compilation-dir=", 22)){
		debug_compilation_dir = arg_substr + 22;
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
		if(!triple_default(&triple)){
			fprintf(stderr, "couldn't get target triple\n");
			return 0;
		}
	}

	switch(triple.arch){
		case ARCH_x86_64:
		case ARCH_i386:
			break;
		default:
			fprintf(stderr, "Only x86_64 architecture is compiled in\n");
			return 0;
	}

	platform_init(triple.arch, triple.sys);
	target_details_from_triple(&triple, &cc1_target_details);

	return 1;
}

int main(int argc, char **argv)
{
	int failure;
	where loc_start;
	static symtable_global *globs;
	const char *in_fname = NULL;
	const char *out_fname = NULL;
	FILE *outfile;
	const char *target = NULL;
	int i;
	int werror = 0;
	dynmap *unknown_warnings = dynmap_new(char *, strcmp, dynmap_strhash);

	/* defaults */
	cc1_mstack_align = -1;
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
			if(argv[i][2] == '\0'){
				cc1_gdebug = DEBUG_FULL;
			}else if(!strcmp(argv[i], "-g0")){
				cc1_gdebug = DEBUG_OFF;
			}else if(!strcmp(argv[i], "-gline-tables-only")){
				cc1_gdebug = DEBUG_LINEONLY;
			}else{
				const char *arg = argv[i] + 2;
				int on = 1;

				if(!strncmp(arg, "no-", 3)){
					arg += 3;
					on = 0;
				}

				if(!strcmp(arg, "column-info"))
					cc1_gdebug_columninfo = on;
				else
					ccdie("Unknown -g switch: \"%s\"", argv[i] + 2);
			}

		}else if(!strcmp(argv[i], "-o")){
			if(++i == argc)
				usage(argv[0], "-o needs an argument");

			if(strcmp(argv[i], "-"))
				out_fname = argv[i];

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

		}else if(argv[i][0] == '-'
		&& (argv[i][1] == 'W' || argv[i][1] == 'f' || argv[i][1] == 'm')){
			parse_Wmf_option(*argv, argv[i], &werror, unknown_warnings);

		}else if(!strncmp(argv[i], "-O", 2)){
			if(optimise(*argv, argv[i] + 2))
				exit(1);

		}else if(!strcmp(argv[i], "-target")){
			i++;
			if(!argv[i]){
				usage(argv[0], "-target requires an argument");
			}
			target = argv[i];

		}else if(!strcmp(argv[i], "--help")){
			dump_options();
			usage(argv[0], NULL);

		}else if(!in_fname){
			in_fname = argv[i];
		}else{
			usage(argv[0], "unknown argument: '%s'", argv[i]);
		}
	}

	if(!init_target(target))
		return 1;

	if(cc1_mstack_align == -1){
		cc1_mstack_align = log2i(platform_word_size());
	}else{
		unsigned new = powf(2, cc1_mstack_align);
		if(new < platform_word_size()){
			ccdie("stack alignment must be >= platform word size (2^%d)",
					log2i(platform_word_size()));
		}
		cc1_mstack_align = new;
	}

	if(werror)
		warnings_upgrade();

	if(warnings_check_unknown(unknown_warnings)){
		failure = 1;
		goto out;
	}

	if(in_fname && strcmp(in_fname, "-")){
		infile = fopen(in_fname, "r");
		if(!infile)
			ccdie("open %s:", in_fname);
	}else{
		infile = stdin;
		in_fname = "-";
	}

	if(out_fname){
		outfile = fopen(out_fname, "w");
		if(!outfile)
			ccdie("open %s:", out_fname);
	}else{
		outfile = stdout;
	}

	show_current_line = cc1_fopt.show_line;

	cc1_type_nav = type_nav_init();

	tokenise_set_mode(
			(cc1_fopt.ext_keywords ? KW_EXT : 0) |
			(cc1_std >= STD_C99 ? KW_C99 : 0));

	tokenise_set_input(next_line, in_fname);

	where_cc1_current(&loc_start);
	globs = symtabg_new(&loc_start);

	failure = parse_and_fold(globs);

	if(fclose(infile))
		ccdie("close input (%s):", in_fname);
	infile = NULL;

	if(failure == 0 || /* attempt dump anyway */cc1_backend == BACKEND_DUMP){
		gen_backend(globs, in_fname, outfile);
		if(gen_had_error)
			failure = 1;
	}

	if(cc1_fopt.dump_type_tree)
		type_nav_dump(cc1_type_nav);

	if(fclose(outfile))
		ccdie("close output (%s):", out_fname);

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
