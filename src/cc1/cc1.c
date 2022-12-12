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
#include "../util/colour.h"

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
#include "sanitize_opt.h"

static const char **system_includes;

struct version
{
	int maj, min;
};

static struct
{
	const char *arg;
	int mask;
} mopts[] = {
	{ "stackrealign", MOPT_STACK_REALIGN },
	{ "align-is-p2", MOPT_ALIGN_IS_POW2 },
	{ "fentry", MOPT_FENTRY },
	{ "red-zone", MOPT_RED_ZONE },

	{ NULL, 0 }
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
	/* note this stores into cc1_mstack_align an invalid value,
	 * that must be 2^n'd before being used */

	{ 0, NULL, NULL }
};

struct cc1_output cc1_output = SECTION_OUTPUT_UNINIT;
dynmap *cc1_outsections;
char *cc1_first_fname;

enum cc1_backend cc1_backend = BACKEND_ASM;

enum mopt mopt_mode = MOPT_RED_ZONE;

enum visibility cc1_visibility_default;

int cc1_mstack_align; /* align stack to n, platform_word_size by default */
int cc1_profileg;
enum debug_level cc1_gdebug = DEBUG_OFF;
int cc1_gdebug_columninfo = 1;

enum stringop_strategy cc1_mstringop_strategy = STRINGOP_STRATEGY_THRESHOLD;
unsigned cc1_mstringop_threshold = 16;

enum c_std cc1_std = STD_C99;

int cc1_error_limit = 16;

int show_current_line;

struct cc1_fopt cc1_fopt;

struct target_details cc1_target_details;
static const char *requested_default_visibility;

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
	fprintf(stderr, "  -g[0|1|2|3], -gline-tables-only|mlt, -g[no-]column-info\n");
	fprintf(stderr, "  -o output-file\n");
	fprintf(stderr, "  -emit=(dump|print|asm|style)\n");
	fprintf(stderr, "  -O[0123s]\n");
	fprintf(stderr, "  --help\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Input options\n");
	fprintf(stderr, "  -pedantic{,-errors}\n");
	fprintf(stderr, "  -W(no-)?(all|extra|everything|gnu|error(=...)|...)\n");
	fprintf(stderr, "  -w\n");
	fprintf(stderr, "  -std=c89/c90/c99/c11/c17/c18 / -ansi / -std=gnu...\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Feature options\n");
	fprintf(stderr, "  -f(sanitize=...|sanitize-error=...|sanitize-undefined-trap-on-error)\n");
	fprintf(stderr, "  -fno-sanitize=all\n");
	fprintf(stderr, "  -fvisibility=default|hidden|protected\n");
	fprintf(stderr, "  -fdebug-compilation-dir=...\n");

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
	fprintf(stderr, "  -mstringop-strategy=(libcall|loop|libcall-threshold=<number>)\n");
	for(i = 0; mopts[i].arg; i++)
		fprintf(stderr, "  -m[no-]%s\n", mopts[i].arg);

	fprintf(stderr, "\n");
	fprintf(stderr, "Feature/machine value options\n");
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

	min->maj = MACOS_VERSION_MAJ;
	min->min = MACOS_VERSION_MIN;
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

static void io_fin_sections(FILE *out)
{
	const struct section *section;
	size_t i;

	if(cc1_gdebug){
		/* ensure we have text and debug-line sections for the debug to reference */
		asm_switch_section(&section_text);
		asm_switch_section(&section_dbg_line);
	}

	for(i = 0; (section = dynmap_key(const struct section *, cc1_outsections, i)); i++){
		if(section_is_builtin(section)){
			const char *desc = asm_section_desc(section->builtin);
			if(desc){
				asm_switch_section(section);
				xfprintf(out, "%s%s%s:\n", cc1_target_details.as->privatelbl_prefix, SECTION_END, desc);
			}
		}
	}
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

static void gen_backend(symtable_global *globs, const char *fname, FILE *out, const char *producer)
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
					&filelist,
					producer);

			/* FIXME: don't take filelist out-param, and free it in gem_asm() */
			/* filelist needs to be output first
			if(0 && filelist && cc1_gdebug != DEBUG_OFF)
				dbg_out_filelist(filelist);
			*/

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

static void set_default_visibility(const char *argv0, const char *visibility)
{
	if(!visibility_parse(&cc1_visibility_default, visibility, cc1_target_details.as->supports_visibility_protected)){
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

	if(invert && arg_ty == 'f' && !strcmp(arg_substr, "sanitize=all")){
		sanitize_opt_off();
		return 1;
	}

	if(invert){
		usage(argv0, "\"no-\" unexpected for value-argument\n");
	}

	if(arg_ty == 'f'){
		if(!strncmp(arg_substr, "sanitize=", 9)){
			sanitize_opt_add(argv0, arg_substr + 9);
			return 1;
		}else if(!strncmp(arg_substr, "sanitize-error=", 15)){
			sanitize_opt_set_error(argv0, arg_substr + 15);
			return 1;
		}else if(!strcmp(arg_substr, "sanitize-undefined-trap-on-error")){
			/* currently the choices are a noreturn function, or trap.
			 * in the future, support could be added for linking with gcc or clang's libubsan,
			 * and calling the runtime support functions therein */
			sanitize_opt_set_error(argv0, "trap");
		}else if(!strncmp(arg_substr, "visibility=", 11)){
			requested_default_visibility = arg_substr + 11;
			return 1;
		}else if(!strncmp(arg_substr, "debug-compilation-dir=", 22)){
			debug_compilation_dir = arg_substr + 22;
			return 1;
		}

	}else if(arg_ty == 'm'){
		if(!strncmp(arg_substr, "stringop-strategy=", 18)){
			const char *strategy = arg_substr + 18;

			/* gcc options are:
			 * rep_byte, rep_4byte, rep_8byte
			 * byte_loop, loop, unrolled_loop
			 * libcall */

			if(!strcmp(strategy, "libcall")){
				cc1_mstringop_strategy = STRINGOP_STRATEGY_LIBCALL;
			}else if(!strcmp(strategy, "loop")){
				cc1_mstringop_strategy = STRINGOP_STRATEGY_LOOP;
			}else if(!strncmp(strategy, "libcall-threshold=", 18)){
				const char *threshold = strategy + 18;
				char *end;

				cc1_mstringop_strategy = STRINGOP_STRATEGY_THRESHOLD;
				cc1_mstringop_threshold = strtol(threshold, &end, 0);
				if(*end)
					usage(argv0, "invalid number for -mmemcpy-strategy=libcall-threshold=..., \"%s\"\n", threshold);
			}else{
				usage(
						argv0,
						"invalid argument to for -mmemcpy-strategy=..., \"%s\", accepted values:\n"
						"  libcall, loop, libcall-threshold=<number>\n"
						, strategy);
			}

			return 1;
		}
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
		unsigned char *opt = fopt_on(&cc1_fopt, arg_substr, invert);
		if(opt){
			if(opt == &cc1_fopt.colour_diagnostics)
				colour_enable(*opt);

			return;
		}
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
		const char *unparsed;
		if(!triple_default(&triple, &unparsed)){
			fprintf(stderr, "couldn't get target triple: %s\n",
					unparsed ? unparsed : strerror(errno));
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

static void output_init(const char *fname)
{
	if(fname){
		cc1_output.file = fopen(fname, "w");
		if(!cc1_output.file)
			ccdie("open %s:", fname);
	}else{
		cc1_output.file = stdout;
	}

	cc1_outsections = dynmap_new(struct section *, section_cmp, section_hash);
}

static void output_term(const char *fname)
{
	struct section *section;
	size_t i;

	for(i = 0; (section = dynmap_key(struct section *, cc1_outsections, i)); i++)
		free(section);

	dynmap_free(cc1_outsections);
	cc1_outsections = NULL;

	if(fclose(cc1_output.file))
		ccdie("close output (%s):", fname);
	cc1_output.file = NULL;
}

int main(int argc, char **argv)
{
	int failure;
	where loc_start;
	static symtable_global *globs;
	const char *in_fname = NULL;
	const char *out_fname = NULL;
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
						usage(argv[0], "-emit needs an argument\n");
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
				usage(argv[0], "unknown emit backend \"%s\"\n", emit);

		}else if(!strncmp(argv[i], "-g", 2)){
			const char *mode = argv[i] + 2;
			int imode;
			char *end;

			if(!*mode){
				cc1_gdebug = DEBUG_FULL;
			}else if((imode = (int)strtol(mode, &end, 0)), !*end){
				switch(imode){
					case 0:
						cc1_gdebug = DEBUG_OFF;
						break;
					case 1:
						cc1_gdebug = DEBUG_LINEONLY;
						break;
					case 2:
					case 3:
						cc1_gdebug = DEBUG_FULL;
						break;
					default:
						goto dbg_unknown;
				}
			}else if(!strcmp(mode, "line-tables-only") || !strcmp(mode, "mlt")){
				cc1_gdebug = DEBUG_LINEONLY;
			}else{
				int on = 1;

				if(!strncmp(mode, "no-", 3)){
					mode += 3;
					on = 0;
				}

				if(!strcmp(mode, "column-info"))
					cc1_gdebug_columninfo = on;
				else
dbg_unknown:
					ccdie("Unknown -g switch: \"%s\"", argv[i] + 2);
			}

		}else if(!strcmp(argv[i], "-o")){
			if(++i == argc)
				usage(argv[0], "-o needs an argument\n");

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

		}else if(!strcmp(argv[i], "-pg")){
			cc1_profileg = 1;

		}else if(!in_fname){
			in_fname = argv[i];
		}else{
			usage(argv[0], "unknown argument: '%s'\n", argv[i]);
		}
	}

	if(!init_target(target))
		return 1;

	if(cc1_mstack_align == -1){
		cc1_mstack_align = platform_word_size();
	}else{
		unsigned new = powf(2, cc1_mstack_align);
		if(new < platform_word_size()){
			ccdie("stack alignment (%d) must be >= %d (platform word size 2^%d)",
					cc1_mstack_align,
					platform_word_size(),
					log2i(platform_word_size()));
		}
		cc1_mstack_align = new;
	}

	if(requested_default_visibility)
		set_default_visibility(argv[0], requested_default_visibility);

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

	output_init(out_fname);

	show_current_line = cc1_fopt.show_line;
	if(cc1_fopt.trapv)
		sanitize_opt_add(argv[0], "signed-integer-overflow");

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
		const char *producer = "ucc development version";

		gen_backend(globs, in_fname, cc1_output.file, producer);
		if(gen_had_error)
			failure = 1;
	}

	if(cc1_fopt.dump_type_tree)
		type_nav_dump(cc1_type_nav);

	output_term(out_fname);

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
