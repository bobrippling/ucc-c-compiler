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
#include "data_structs.h"
#include "tokenise.h"
#include "parse.h"
#include "cc1.h"
#include "fold.h"
#include "gen_asm.h"
#include "gen_str.h"
#include "gen_style.h"
#include "sym.h"
#include "fold_sym.h"
#include "out/out.h"
#include "ops/__builtin.h"
#include "pass1.h"

#include "../as_cfg.h"
#define QUOTE(...) #__VA_ARGS__
#define EXPAND_QUOTE(y) QUOTE(y)

struct
{
	int is_opt;
	const char *arg;
	int mask;
} args[] = {
	/* TODO - wall picks sensible, extra = more, everything = ~0 */
	{ 0,  "all",             ~0 },
	{ 0,  "extra",            0 },
	{ 0,  "everything",      ~0 },


	{ 0,  "mismatch-arg",    WARN_ARG_MISMATCH                      },
	{ 0,  "array-comma",     WARN_ARRAY_COMMA                       },
	{ 0,  "mismatch-assign", WARN_ASSIGN_MISMATCH | WARN_COMPARE_MISMATCH | WARN_RETURN_TYPE },
	{ 0,  "return-type",     WARN_RETURN_TYPE                       },

	{ 0,  "sign-compare",    WARN_SIGN_COMPARE                      },
	{ 0,  "extern-assume",   WARN_EXTERN_ASSUME                     },

	{ 0,  "implicit-int",    WARN_IMPLICIT_INT                      },
	{ 0,  "implicit-func",   WARN_IMPLICIT_FUNC                     },
	{ 0,  "implicit",        WARN_IMPLICIT_FUNC | WARN_IMPLICIT_INT },

	{ 0,  "switch-enum",     WARN_SWITCH_ENUM                       },
	{ 0,  "enum-compare",    WARN_ENUM_CMP                          },

	{ 0,  "incomplete-use",  WARN_INCOMPLETE_USE                    },

	{ 0,  "unused-expr",     WARN_UNUSED_EXPR                       },

	{ 0,  "test-in-assign",  WARN_TEST_ASSIGN                       },
	{ 0,  "test-bool",       WARN_TEST_BOOL                         },

	{ 0,  "dead-code",       WARN_DEAD_CODE                         },

	{ 0,  "predecl-enum",    WARN_PREDECL_ENUM,                     },


	{ 0, "mixed-code-decls", WARN_MIXED_CODE_DECLS                  },

	{ 0, "loss-of-precision", WARN_LOSS_PRECISION                   },

	{ 0, "pad",               WARN_PAD },

	{ 0, "tenative-init",     WARN_TENATIVE_INIT },

	{ 0, "shadow-local",      WARN_SHADOW_LOCAL },
	{ 0, "shadow-global",     WARN_SHADOW_GLOBAL },
	{ 0, "shadow",            WARN_SHADOW_GLOBAL | WARN_SHADOW_LOCAL },

	/* TODO: W_QUAL (ops/expr_cast) */

#if 0
	/* TODO */
	{ 0,  "unused-parameter", WARN_UNUSED_PARAM },
	{ 0,  "unused-variable",  WARN_UNUSED_VAR   },
	{ 0,  "unused-value",     WARN_UNUSED_VAL   },
	{ 0,  "unused",           WARN_UNUSED_PARAM | WARN_UNUSED_VAR | WARN_UNUSED_VAL },

	/* TODO */
	{ 0,  "uninitialised",    WARN_UNINITIALISED },

	/* TODO */
	{ 0,  "array-bounds",     WARN_ARRAY_BOUNDS },

	/* TODO */
	{ 0,  "shadow",           WARN_SHADOW },

	/* TODO */
	{ 0,  "format",           WARN_FORMAT },

	/* TODO */
	{ 0,  "pointer-arith",    WARN_PTR_ARITH  }, /* void *x; x++; */
	{ 0,  "int-ptr-cast",     WARN_INT_TO_PTR },
#endif

	{ 0,  "optimisation",     WARN_OPT_POSSIBLE },


/* --- options --- */

	{ 1,  "enable-asm",    FOPT_ENABLE_ASM      },
	{ 1,  "const-fold",    FOPT_CONST_FOLD      },
	{ 1,  "english",       FOPT_ENGLISH         },
	{ 1,  "show-line",     FOPT_SHOW_LINE       },
	{ 1,  "pic",           FOPT_PIC             },
	{ 1,  "pic-pcrel",     FOPT_PIC_PCREL       },
	{ 1,  "builtin",       FOPT_BUILTIN         },
	{ 1,  "ms-extensions",    FOPT_MS_EXTENSIONS    },
	{ 1,  "plan9-extensions", FOPT_PLAN9_EXTENSIONS },
	{ 1,  "leading-underscore", FOPT_LEADING_UNDERSCORE },
	{ 1,  "trapv",              FOPT_TRAPV },
	{ 1,  "track-initial-fname", FOPT_TRACK_INITIAL_FNAM },
	{ 1,  "freestanding",        FOPT_FREESTANDING },
	{ 1,  "show-static-asserts", FOPT_SHOW_STATIC_ASSERTS },

	{ 0,  NULL, 0 }
};

struct
{
	char pref;
	const char *arg;
	int *pval;
} val_args[] = {
	{ 'f', "max-errors",   &cc1_max_errors },
	{ 'm', "preferred-stack-boundary", &cc1_mstack_align },
	{ 0, NULL, NULL }
};

FILE *cc_out[NUM_SECTIONS];     /* temporary section files */
char  fnames[NUM_SECTIONS][32]; /* duh */
FILE *cc1_out;                  /* final output */

enum warning warn_mode = ~(
		  WARN_VOID_ARITH
		| WARN_IMPLICIT_INT
		| WARN_LOSS_PRECISION
		| WARN_SIGN_COMPARE
		| WARN_PAD
		| WARN_TENATIVE_INIT
		| WARN_SHADOW_GLOBAL
		);

enum fopt fopt_mode = FOPT_CONST_FOLD
                    | FOPT_SHOW_LINE
                    | FOPT_PIC
                    | FOPT_BUILTIN
                    | FOPT_TRACK_INITIAL_FNAM;
enum cc1_backend cc1_backend = BACKEND_ASM;

int cc1_m32 = UCC_M32;
int cc1_mstack_align; /* align stack to n, platform_word_size by default */
int cc1_gdebug;

enum c_std cc1_std = STD_C99;

int cc1_max_errors = 16;

int caught_sig = 0;

int show_current_line;

const char *section_names[NUM_SECTIONS] = {
	EXPAND_QUOTE(SECTION_TEXT),
	EXPAND_QUOTE(SECTION_DATA),
	EXPAND_QUOTE(SECTION_BSS),
};

static FILE *infile;

/* compile time check for enum <-> int compat */
#define COMP_CHECK(pre, test) \
struct unused_ ## pre { char check[test ? -1 : 1]; }
COMP_CHECK(a, sizeof warn_mode != sizeof(int));
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
		for(i = 0; args[i].arg; i++)
			fprintf(stderr, "  -%c%s\n", args[i].is_opt["Wf"], args[i].arg);
		for(i = 0; val_args[i].arg; i++)
			fprintf(stderr, "  -%c%s=value\n", val_args[i].pref, val_args[i].arg);
	}

	exit(1);
}

void cc1_warn_atv(struct where *where, int die, enum warning w, const char *fmt, va_list l)
{
	if(!die && (w & warn_mode) == 0)
		return;

	vwarn(where, die, fmt, l);

	if(die)
		exit(1);
}

void cc1_warn_at(struct where *where, int die, enum warning w, const char *fmt, ...)
{
	va_list l;

	va_start(l, fmt);
	cc1_warn_atv(where, die, w, fmt, l);
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

#if 0
	if(do_sections){
		if(fprintf(cc1_out, "\t.file \"%s\"\n", fname) < 0)
			ccdie(0, "write to cc1_out:");
	}
#else
	(void)fname;
#endif

	for(i = 0; i < NUM_SECTIONS; i++){
		/* cat cc_out[i] to cc1_out, with section headers */
		if(do_sections){
			char buf[256];
			long last = ftell(cc_out[i]);

			if(last == -1 || fseek(cc_out[i], 0, SEEK_SET) == -1)
				ccdie(0, "seeking on section file %d:", i);

			if(fprintf(cc1_out, ".section %s\n", section_names[i]) < 0)
				ccdie(0, "write to cc1 output:");

			while(fgets(buf, sizeof buf, cc_out[i]))
				if(fputs(buf, cc1_out) == EOF)
					ccdie(0, "write to cc1 output:");

			if(ferror(cc_out[i]))
				ccdie(0, "read from section file %d:", i);
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

int main(int argc, char **argv)
{
	static symtable_global *globs;
	void (*gf)(symtable_global *);
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
	cc1_mstack_align = log2f(platform_word_size());

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
			if(std_from_str(argv[i], &cc1_std))
				ccdie(0, "-std argument \"%s\" not recognised", argv[i]);

		}else if(!strcmp(argv[i], "-w")){
			warn_mode = WARN_NONE;

		}else if(!strcmp(argv[i], "-Werror")){
			werror = 1;

		}else if(argv[i][0] == '-'
		&& (argv[i][1] == 'W' || argv[i][1] == 'f' || argv[i][1] == 'm')){
			const char arg_ty = argv[i][1];
			char *arg = argv[i] + 2;
			int *mask;
			int j, found, rev;

			j = rev = found = 0;

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

				mask = (int *)&fopt_mode;
				for(; !args[j].is_opt; j++);
			}else{
				mask = (int *)&warn_mode;
			}

			for(; args[j].arg; j++)
				if(!strcmp(arg, args[j].arg)){
					if(rev)
						*mask &= ~args[j].mask;
					else
						*mask |=  args[j].mask;
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

			cc1_m32 = n == 32;

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
					(int)log2f(platform_word_size()));

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

	switch(cc1_backend){
		case BACKEND_ASM:   gf = gen_asm;   break;
		case BACKEND_STYLE: gf = gen_style; break;
		case BACKEND_PRINT: gf = gen_str;   break;
	}

	io_setup();

	show_current_line = fopt_mode & FOPT_SHOW_LINE;

	globs = symtabg_new();
	tokenise_set_input(next_line, fname);

	parse_and_fold(globs);

	if(infile != stdin)
		fclose(infile), infile = NULL;

	if(werror && warning_count)
		ccdie(0, "%s: Treating warnings as errors", *argv);

	gf(globs);

	io_fin(gf == gen_asm, fname);

	return 0;
}
