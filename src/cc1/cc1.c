#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

#include <unistd.h>
#include <signal.h>

#include "../util/util.h"
#include "../util/platform.h"
#include "../util/math.h"

#include "tokenise.h"
#include "cc1.h"
#include "fold.h"
#include "gen_asm.h"
#include "gen_str.h"
#include "gen_style.h"
#include "sym.h"
#include "fold_sym.h"
#include "ops/__builtin.h"
#include "out/asm.h" /* NUM_SECTIONS */
#include "pass1.h"
#include "type_nav.h"
#include "cc1_where.h"

#include "../as_cfg.h"

struct cc1_warning cc1_warning;

enum warning_special
{
	W_ALL, W_EXTRA, W_EVERYTHING
};

static struct warn_str
{
	const char *arg;
	unsigned char *offsets[3];
} warns[] = {
	{ "mismatch-arg", &cc1_warning.arg_mismatch },
	{ "array-comma", &cc1_warning.array_comma },
	{ "return-type", &cc1_warning.return_type },

	{
		"mismatch-assign",
		&cc1_warning.assign_mismatch,
		&cc1_warning.compare_mismatch,
		&cc1_warning.return_type
	},

	{ "sign-compare", &cc1_warning.sign_compare },
	{ "extern-assume", &cc1_warning.extern_assume },

	{ "implicit-int", &cc1_warning.implicit_int },
	{ "implicit-func", &cc1_warning.implicit_func },
	{ "implicit", &cc1_warning.implicit_func, &cc1_warning.implicit_int },

	{ "switch-enum", &cc1_warning.switch_enum },
	{ "enum-compare", &cc1_warning.enum_cmp },

	{ "incomplete-use", &cc1_warning.incomplete_use },

	{ "unused-expr", &cc1_warning.unused_expr },

	{ "test-in-assign", &cc1_warning.test_assign },
	{ "test-bool", &cc1_warning.test_bool },

	{ "dead-code", &cc1_warning.dead_code },

	{ "predecl-enum", &cc1_warning.predecl_enum, },

	{ "mixed-code-decls", &cc1_warning.mixed_code_decls },

	{ "loss-of-precision", &cc1_warning.loss_precision },

	{ "pad", &cc1_warning.pad },

	{ "tenative-init", &cc1_warning.tenative_init },

	{ "shadow-local", &cc1_warning.shadow_local },
	{ "shadow-global", &cc1_warning.shadow_global },
	{ "shadow", &cc1_warning.shadow_global, &cc1_warning.shadow_local },

	{ "cast-qual", &cc1_warning.cast_qual },

	{ "unused-parameter", &cc1_warning.unused_param },
	{ "unused-variable", &cc1_warning.unused_var },
	{ "unused-value", &cc1_warning.unused_val },
	{
		"unused",
		&cc1_warning.unused_param,
		&cc1_warning.unused_var,
		&cc1_warning.unused_val
	},

	{ "uninitialised", &cc1_warning.uninitialised },

	{ "int-ptr-conversion", &cc1_warning.int_ptr_conv },
	{ "ptr-int-conversion", &cc1_warning.int_ptr_conv },

	{ "arith-funcptr", &cc1_warning.arith_fnptr },
	{ "arith-voidptr", &cc1_warning.arith_voidp },

	{ "array-bounds", &cc1_warning.array_oob },
	{ "asm-badchar", &cc1_warning.asm_badchar },

	{ "attr-badcleanup", &cc1_warning.attr_badcleanup },
	{ "format", &cc1_warning.format },
	{ "attr-format-baddecl", &cc1_warning.attr_format_baddecl },
	{ "attr-format-unknown", &cc1_warning.attr_format_unknown },
	{ "attr-noderef", &cc1_warning.attr_noderef },
	{ "attr-nonnull", &cc1_warning.attr_nonnull },
	{ "attr-nonnull-bad", &cc1_warning.attr_nonnull_bad },
	{ "attr-nonnull-noargs", &cc1_warning.attr_nonnull_noargs },
	{ "attr-nonnull-nonptr", &cc1_warning.attr_nonnull_nonptr },
	{ "attr-nonnull-oob", &cc1_warning.attr_nonnull_oob },
	{ "attr-printf-bad", &cc1_warning.attr_printf_bad },
	{ "attr-printf-toomany", &cc1_warning.attr_printf_toomany },
	{ "attr-printf-unknown", &cc1_warning.attr_printf_unknown },
	{ "attr-section-badchar", &cc1_warning.attr_section_badchar },
	{ "attr-sentinel", &cc1_warning.attr_sentinel },
	{ "attr-sentinel-nonvariadic", &cc1_warning.attr_sentinel_nonvariadic },
	{ "attr-unknown", &cc1_warning.attr_unknown },
	{ "attr-unused-used", &cc1_warning.attr_unused_used },
	{ "attr-unused-voidfn", &cc1_warning.attr_unused_voidfn },

	{ "qualified-fntype", &cc1_warning.bad_funcqual },
	{ "inline-non-function", &cc1_warning.bad_inline },
	{ "restrict-non-ptr", &cc1_warning.bad_restrict },

	{ "bitfield-boundary", &cc1_warning.bitfield_boundary },
	{ "bitfield-onebit-int", &cc1_warning.bitfield_onebit_int },
	{ "bitfield-trunc", &cc1_warning.bitfield_trunc },

	{ "builtin-expect-nonconst", &cc1_warning.builtin_expect_nonconst },
	{ "builtin-memset-bad", &cc1_warning.builtin_memset_bad },
	{ "builtin-va_arg-undefined", &cc1_warning.builtin_va_arg },
	{ "builtin-va_start-2nd-param", &cc1_warning.builtin_va_start },

	{ "c89-compound-literal", &cc1_warning.c89_compound_literal },
	{ "c89-for-init", &cc1_warning.c89_for_init },
	{ "c89-init-constexpr", &cc1_warning.c89_init_constexpr },
	{ "c89-trailing-comma", &cc1_warning.c89_parse_trailingcomma },

	{ "constant-large-unsigned", &cc1_warning.constant_large_unsigned },

	{ "div-zero", &cc1_warning.constop_bad },
	{ "declaration-noop", &cc1_warning.decl_nodecl },
	{ "empty-struct", &cc1_warning.empty_struct },

	{ "enum-mismatch", &cc1_warning.enum_mismatch },
	{ "enum-switch-bitmask", &cc1_warning.enum_switch_bitmask },
	{ "enum-switch-imposter", &cc1_warning.enum_switch_imposter },

	{ "excess-init", &cc1_warning.excess_init },
	{ "extern-init", &cc1_warning.extern_init },

	{ "embedded-flexarr", &cc1_warning.flexarr_embed },
	{ "flexarr-single-member", &cc1_warning.flexarr_only },
	{ "flexarr-init", &cc1_warning.flexarr_init },

	{ "call-argcount", &cc1_warning.funcall_argcount },

	{ "ignored-late-decl", &cc1_warning.ignored_late_decl },

	{ "implicit-old-func", &cc1_warning.implicit_old_func },

	{ "init-missing-braces", &cc1_warning.init_missing_braces },
	{ "init-missing-struct", &cc1_warning.init_missing_struct },
	{ "init-obj-discard", &cc1_warning.init_obj_discard },
	{ "init-overlong-string", &cc1_warning.init_overlong_strliteral },
	{ "init-override", &cc1_warning.init_override },


	{ "unknown-attribute", &cc1_warning.lbl_attr_unknown },
	{ "unused-label", &cc1_warning.lbl_unused },

	{ "long-long", &cc1_warning.long_long },

	{ "mismatch-conditional", &cc1_warning.mismatch_conditional },
	{ "mismatch-ptr", &cc1_warning.mismatch_ptr },
	{ "mismatching-types", &cc1_warning.mismatching_types },

	{ "missing-empty-struct-brace-init", &cc1_warning.missing_empty_struct_brace_init },

	{ "multichar", &cc1_warning.multichar },
	{ "multichar-too-large", &cc1_warning.multichar_toolarge },

	{ "nonstandard-array-size", &cc1_warning.nonstd_arraysz },
	{ "nonstandard-init", &cc1_warning.nonstd_init },

	{ "omitted-param-types", &cc1_warning.omitted_param_types },
	{ "undefined-shift", &cc1_warning.op_shift_bad },
	{ "overlarge-enumerator", &cc1_warning.overlarge_enumerator },

	{ "operator-precedence", &cc1_warning.parse_precedence },
	{ "visibility", &cc1_warning.private_struct },

	{ "pure-inline", &cc1_warning.pure_inline },
	{ "restrict-ptrs", &cc1_warning.restrict_ptrs },
	{ "return-undef", &cc1_warning.return_undef },
	{ "signed-unsigned", &cc1_warning.signed_unsigned },
	{ "sizeof-decayed", &cc1_warning.sizeof_decayed },
	{ "static-array-size", &cc1_warning.static_array_bad },
	{ "str-contain-nul", &cc1_warning.str_contain_nul },

	{ "struct-noinstance-anon", &cc1_warning.struct_noinstance_anon },
	{ "struct-noinstance-qualified", &cc1_warning.struct_noinstance_qualified },

	{ "sym-never-read", &cc1_warning.sym_never_read },
	{ "sym-never-written", &cc1_warning.sym_never_written },

	{ "tautologic-unsigned-op", &cc1_warning.tautologic_unsigned },
	{ "tenative-array", &cc1_warning.tenative_array_1elem },

	{ "typedef-function-impl", &cc1_warning.typedef_fnimpl },
	{ "typedef-redefinition", &cc1_warning.typedef_redef },

	{ "undef-string-comparison", &cc1_warning.undef_strlitcmp },

	{ "unnamed-struct-memb", &cc1_warning.unnamed_struct_memb },
	{ "unused-comma", &cc1_warning.unused_comma },

	{ "vla", &cc1_warning.vla },

	{ "__func__init", &cc1_warning.x__func__init },
	{ "__func__outside-fn", &cc1_warning.x__func__outsidefn },

	{ NULL, NULL }
};


static struct
{
	char type;
	const char *arg;
	int mask;
} fopts[] = {
	{ 'f',  "enable-asm",    FOPT_ENABLE_ASM      },
	{ 'f',  "const-fold",    FOPT_CONST_FOLD      },
	{ 'f',  "english",       FOPT_ENGLISH         },
	{ 'f',  "show-line",     FOPT_SHOW_LINE       },
	{ 'f',  "pic",           FOPT_PIC             },
	{ 'f',  "pic-pcrel",     FOPT_PIC_PCREL       },
	{ 'f',  "builtin",       FOPT_BUILTIN         },
	{ 'f',  "ms-extensions",    FOPT_MS_EXTENSIONS    },
	{ 'f',  "plan9-extensions", FOPT_PLAN9_EXTENSIONS },
	{ 'f',  "leading-underscore", FOPT_LEADING_UNDERSCORE },
	{ 'f',  "trapv",              FOPT_TRAPV },
	{ 'f',  "track-initial-fname", FOPT_TRACK_INITIAL_FNAM },
	{ 'f',  "freestanding",        FOPT_FREESTANDING },
	{ 'f',  "show-static-asserts", FOPT_SHOW_STATIC_ASSERTS },
	{ 'f',  "verbose-asm",         FOPT_VERBOSE_ASM },
	{ 'f',  "integral-float-load", FOPT_INTEGRAL_FLOAT_LOAD },
	{ 'f',  "symbol-arith",        FOPT_SYMBOL_ARITH },
	{ 'f',  "signed-char",         FOPT_SIGNED_CHAR },
	{ 'f',  "unsigned-char",      ~FOPT_SIGNED_CHAR },
	{ 'f',  "cast-with-builtin-types", FOPT_CAST_W_BUILTIN_TYPES },
	{ 'f',  "dump-type-tree", FOPT_DUMP_TYPE_TREE },
	{ 'f',  "asm", FOPT_EXT_KEYWORDS },
	{ 'f',  "fold-const-vlas", FOPT_FOLD_CONST_VLAS },

	{ 'm',  "stackrealign", MOPT_STACK_REALIGN },

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
static char fnames[NUM_SECTIONS][32]; /* duh */
FILE *cc1_out;                  /* final output */
char *cc1_first_fname;

enum fopt fopt_mode = FOPT_CONST_FOLD
                    | FOPT_SHOW_LINE
                    | FOPT_PIC
                    | FOPT_BUILTIN
                    | FOPT_TRACK_INITIAL_FNAM
                    | FOPT_INTEGRAL_FLOAT_LOAD
                    | FOPT_SYMBOL_ARITH
                    | FOPT_SIGNED_CHAR
                    | FOPT_CAST_W_BUILTIN_TYPES
                    | FOPT_FOLD_CONST_VLAS;

enum cc1_backend cc1_backend = BACKEND_ASM;

enum mopt mopt_mode = 0;

int cc1_mstack_align; /* align stack to n, platform_word_size by default */
int cc1_gdebug;

enum c_std cc1_std = STD_C99;

int cc1_error_limit = 16;

static int caught_sig = 0;

int show_current_line;

struct section sections[NUM_SECTIONS] = {
	{ "text", QUOTE(SECTION_NAME_TEXT) },
	{ "data", QUOTE(SECTION_NAME_DATA) },
	{ "bss",  QUOTE(SECTION_NAME_BSS) },
	{ "rodata", QUOTE(SECTION_NAME_RODATA) },
	{ "dbg_abrv", QUOTE(SECTION_NAME_DBG_ABBREV) },
	{ "dbg_info", QUOTE(SECTION_NAME_DBG_INFO) },
	{ "dbg_line", QUOTE(SECTION_NAME_DBG_LINE) },
};

static FILE *infile;

/* compile time check for enum <-> int compat */
#define COMP_CHECK(pre, test) \
struct unused_ ## pre { char check[test ? -1 : 1]; }
COMP_CHECK(b, sizeof fopt_mode != sizeof(int));


static void ccdie(int verbose, const char *fmt, ...)
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

	if(verbose){
		fputs("warnings + options:\n", stderr);
		for(i = 0; fopts[i].arg; i++)
			fprintf(stderr, "  -%c%s\n", fopts[i].type, fopts[i].arg);
		for(i = 0; val_args[i].arg; i++)
			fprintf(stderr, "  -%c%s=value\n", val_args[i].pref, val_args[i].arg);
	}

	exit(1);
}

#undef cc1_warn_at
void cc1_warn_at(struct where *where, const char *fmt, ...)
{
	va_list l;

	va_start(l, fmt);
	vwarn(where, 0, fmt, l);
	va_end(l);
}

static void io_cleanup(void)
{
	int i;
	for(i = 0; i < NUM_SECTIONS; i++){
		if(!cc_out[i])
			continue;

		if(fclose(cc_out[i]) == EOF && !caught_sig)
			fprintf(stderr, "close %s: %s\n", fnames[i], strerror(errno));
		if(remove(fnames[i]) && !caught_sig)
			fprintf(stderr, "remove %s: %s\n", fnames[i], strerror(errno));
	}
}

static void io_setup(void)
{
	int i;

	if(!cc1_out)
		cc1_out = stdout;

	for(i = 0; i < NUM_SECTIONS; i++){
		snprintf(fnames[i], sizeof fnames[i], "/tmp/cc1_%d%d", getpid(), i);

		cc_out[i] = fopen(fnames[i], "w+"); /* need to seek */
		if(!cc_out[i])
			ccdie(0, "open \"%s\":", fnames[i]);
	}

	atexit(io_cleanup);
}

static void io_fin(int do_sections, const char *fname)
{
	int i;

	(void)fname;

	for(i = 0; i < NUM_SECTIONS; i++){
		/* cat cc_out[i] to cc1_out, with section headers */
		if(do_sections){
			char buf[256];
			long last = ftell(cc_out[i]);

			if(last == -1 || fseek(cc_out[i], 0, SEEK_SET) == -1)
				ccdie(0, "seeking on section file %d:", i);

			if(fprintf(cc1_out, ".section %s\n", sections[i].name) < 0
			|| fprintf(cc1_out, "%s%s:\n", SECTION_BEGIN, sections[i].desc) < 0)
			{
				ccdie(0, "write to cc1 output:");
			}

			while(fgets(buf, sizeof buf, cc_out[i]))
				if(fputs(buf, cc1_out) == EOF)
					ccdie(0, "write to cc1 output:");

			if(ferror(cc_out[i]))
				ccdie(0, "read from section file %d:", i);

			if(fprintf(cc1_out, "%s%s:\n", SECTION_END, sections[i].desc) < 0)
				ccdie(0, "terminating section %d:", i);
		}
	}

	if(fclose(cc1_out))
		ccdie(0, "close cc1 output");
}

static void sigh(int sig)
{
	(void)sig;
	caught_sig = 1;
	io_cleanup();
}

static char *next_line()
{
	char *s = fline(infile);
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
				case BACKEND_PRINT:
					gf = gen_str;
			}
			gf(globs);
			break;

		case BACKEND_ASM:
			{
				char buf[4096];
				char *compdir;

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
						compdir);

				if(compdir != buf)
					free(compdir);
				break;
			}
	}

	io_fin(gf == NULL, fname);
}

static void warnings_set(int to)
{
	memset(&cc1_warning, to, sizeof cc1_warning);
}

static void warning_all(void)
{
	warnings_set(1);

	cc1_warning.implicit_int =
	cc1_warning.loss_precision =
	cc1_warning.sign_compare =
	cc1_warning.pad =
	cc1_warning.tenative_init =
	cc1_warning.shadow_global =
	cc1_warning.implicit_old_func =
	cc1_warning.bitfield_boundary =
	cc1_warning.vla =
		0;
}

static void warning_init(void)
{
	/* default to -Wall */
	warning_all();
}

static void warning_special(enum warning_special type)
{
	switch(type){
		case W_EVERYTHING:
			warnings_set(1);
			break;
		case W_ALL:
			warning_all();
			break;
		case W_EXTRA:
			warning_all();
			cc1_warning.implicit_int =
			cc1_warning.loss_precision =
			cc1_warning.sign_compare =
			cc1_warning.tenative_init =
			cc1_warning.shadow_global = 1;
			break;
	}
}

static void warning_on(const char *warn, int to)
{
	struct warn_str *p;

#define SPECIAL(str, w)   \
	if(!strcmp(warn, str)){ \
		warning_special(w);   \
		return;               \
	}

	SPECIAL("all", W_ALL)
	SPECIAL("extra", W_EXTRA)
	SPECIAL("everything", W_EVERYTHING)

	for(p = warns; p->arg; p++){
		if(!strcmp(warn, p->arg)){
			unsigned i;
			for(i = 0; i < sizeof(p->offsets)/sizeof(p->offsets[0]); i++){
				if(!p->offsets[i])
					break;

				*p->offsets[i] = to;
			}
			return;
		}
	}

	fprintf(stderr, "Unknown warning option \"-W%s\"\n", warn);
}

int main(int argc, char **argv)
{
	int parsed_folded;
	where loc_start;
	static symtable_global *globs;
	const char *fname;
	int i;
	int werror = 0;

	/*signal(SIGINT , sigh);*/
	signal(SIGQUIT, sigh);
	signal(SIGTERM, sigh);
	signal(SIGABRT, sigh);
	signal(SIGSEGV, sigh);

	fname = NULL;

	/* defaults */
	cc1_mstack_align = log2i(platform_word_size());
	warning_init();

	for(i = 1; i < argc; i++){
		if(!strcmp(argv[i], "-X")){
			if(++i == argc)
				goto usage;

			if(!strcmp(argv[i], "print"))
				cc1_backend = BACKEND_PRINT;
			else if(!strcmp(argv[i], "asm"))
				cc1_backend = BACKEND_ASM;
			else if(!strcmp(argv[i], "style"))
				cc1_backend = BACKEND_STYLE;
			else
				goto usage;

		}else if(!strcmp(argv[i], "-g")){
			cc1_gdebug = 1;

		}else if(!strcmp(argv[i], "-o")){
			if(++i == argc)
				goto usage;

			if(strcmp(argv[i], "-")){
				cc1_out = fopen(argv[i], "w");
				if(!cc1_out){
					ccdie(0, "open %s:", argv[i]);
					return 1;
				}
			}

		}else if(!strncmp(argv[i], "-std=", 5) || !strcmp(argv[i], "-ansi")){
			int gnu;

			if(std_from_str(argv[i], &cc1_std, &gnu))
				ccdie(0, "-std argument \"%s\" not recognised", argv[i]);

			if(gnu)
				fopt_mode |= FOPT_EXT_KEYWORDS;
			else
				fopt_mode &= ~FOPT_EXT_KEYWORDS;

		}else if(!strcmp(argv[i], "-w")){
			warnings_set(0);

		}else if(!strcmp(argv[i], "-Werror")){
			werror = 1;

		}else if(argv[i][0] == '-'
		&& (argv[i][1] == 'W' || argv[i][1] == 'f' || argv[i][1] == 'm')){
			const char arg_ty = argv[i][1];
			char *arg = argv[i] + 2;
			int *mask;
			int j, found, rev;

			rev = found = 0;

			if(!strncmp(arg, "no-", 3)){
				arg += 3;
				rev = 1;
			}

			if(arg_ty != 'W'){
				char *equal = strchr(arg, '=');

				if(equal){
					int new_val;

					if(rev){
						fprintf(stderr, "\"no-\" unexpected for value-argument\n");
						goto usage;
					}

					*equal = '\0';
					if(sscanf(equal + 1, "%d", &new_val) != 1){
						fprintf(stderr, "need number for %s\n", arg);
						goto usage;
					}

					for(j = 0; val_args[j].arg; j++)
						if(val_args[j].pref == arg_ty && !strcmp(arg, val_args[j].arg)){
							*val_args[j].pval = new_val;
							found = 1;
							break;
						}

					if(!found)
						goto unrecognised;
					continue;
				}
			}

			switch(arg_ty){
				case 'f':
					mask = (int *)&fopt_mode;
					break;
				case 'm':
					mask = (int *)&mopt_mode;
					break;
				default:
					ucc_unreach(1);

				case 'W':
					warning_on(arg, !rev);
					continue;
			}

			for(j = 0; fopts[j].arg; j++)
				if(fopts[j].type == arg_ty && !strcmp(arg, fopts[j].arg)){
					/* if the mask isn't a single bit, treat it as
					 * an unmask, e.g. -funsigned-char unmasks FOPT_SIGNED_CHAR
					 */
					const int unmask = fopts[j].mask & (fopts[j].mask - 1);

					if(rev){
						if(unmask)
							*mask |= ~fopts[j].mask;
						else
							*mask &= ~fopts[j].mask;
					}else{
						if(unmask)
							*mask &= fopts[j].mask;
						else
							*mask |= fopts[j].mask;
					}
					found = 1;
					break;
				}

			if(!found){
unrecognised:
				fprintf(stderr, "\"%s\" unrecognised\n", argv[i]);
				goto usage;
			}

		}else if(!strncmp(argv[i], "-m", 2)){
			int n;

			if(sscanf(argv[i] + 2, "%d", &n) != 1 || (n != 32 && n != 64)){
				fprintf(stderr, "-m needs either 32 or 64\n");
				goto usage;
			}

			if(n == 32)
				mopt_mode |= MOPT_32;
			else
				mopt_mode &= ~MOPT_32;

		}else if(!fname){
			fname = argv[i];
		}else{
usage:
			ccdie(1, "Usage: %s [-W[no-]warning] [-f[no-]option] [-X backend] [-m[32|64]] [-o output] file", *argv);
		}
	}

	/* sanity checks */
	{
		const unsigned new = powf(2, cc1_mstack_align);
		if(new < platform_word_size())
			ccdie(0, "stack alignment must be >= platform word size (2^%d)",
					log2i(platform_word_size()));

		cc1_mstack_align = new;
	}

	if(fname && strcmp(fname, "-")){
		infile = fopen(fname, "r");
		if(!infile)
			ccdie(0, "open %s:", fname);
	}else{
		infile = stdin;
		fname = "-";
	}

	io_setup();

	show_current_line = fopt_mode & FOPT_SHOW_LINE;

	cc1_type_nav = type_nav_init();

	tokenise_set_mode(
			(fopt_mode & FOPT_EXT_KEYWORDS ? KW_EXT : 0) |
			(cc1_std >= STD_C99 ? KW_C99 : 0));

	tokenise_set_input(next_line, fname);

	where_cc1_current(&loc_start);
	globs = symtabg_new(&loc_start);

	parsed_folded = parse_and_fold(globs);

	if(infile != stdin)
		fclose(infile), infile = NULL;

	if(werror && warning_count)
		ccdie(0, "%s: Treating warnings as errors", *argv);

	if(parsed_folded == 0)
		gen_backend(globs, fname);

	if(fopt_mode & FOPT_DUMP_TYPE_TREE)
		type_nav_dump(cc1_type_nav);

	return parsed_folded;
}
