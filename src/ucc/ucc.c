#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <assert.h>

/* umask */
#include <sys/types.h>
#include <sys/stat.h>

#include "cfg.h"

#include "ucc.h"
#include "ucc_ext.h"
#include "ucc_lib.h"
#include "../util/alloc.h"
#include "../util/dynarray.h"
#include "../util/util.h"
#include "../util/platform.h"
#include "str.h"

enum mode
{
	mode_preproc,
	mode_compile,
	mode_assemb,
	mode_link
};
#define MODE_ARG_CH(m) ("ESc\0"[m])

struct
{
	enum mode assume;
	int nostdlib;
	int nostartfiles;
} gopts;


struct cc_file
{
	char *in;

	char *preproc,
			 *compile,
			 *assemb;

	char *out;

	int preproc_asm;
};
#define FILE_IN_MODE(f)        \
((f)->preproc ? mode_preproc : \
 (f)->compile ? mode_compile : \
 (f)->assemb  ? mode_assemb  : \
 mode_link)

static char **remove_these;
static int unlink_tmps = 1;
static int gdebug = 0;
const char *argv0;
char *wrapper;

static void unlink_files(void)
{
	int i;
	for(i = 0; remove_these[i]; i++){
		if(unlink_tmps)
			remove(remove_these[i]);
		free(remove_these[i]);
	}
	free(remove_these);
}

static char *tmpfilenam()
{
	char *r = tmpnam(NULL);

	if(!r)
		die("tmpfile():");

	r = ustrdup(r);

	if(!remove_these) /* only register once */
		atexit(unlink_files);

	dynarray_add(&remove_these, r);

	return r;
}

static void create_file(struct cc_file *file, enum mode mode, char *in)
{
	char *ext;

	file->in = in;

	if(!strcmp(in, "-"))
		goto preproc;

	switch(gopts.assume){
		case mode_preproc:
			goto preproc;
		case mode_compile:
			goto compile;
		case mode_assemb:
			goto assemb;
		case mode_link:
			goto assume_obj;
	}

#define ASSIGN(x)                \
			if(!file->x){              \
				file->x = tmpfilenam();  \
				if(mode == mode_ ## x){  \
					file->out = file->x;   \
					return;                \
				}                        \
			}

	ext = strrchr(in, '.');
	if(ext && ext[1] && !ext[2]){
		switch(ext[1]){
preproc:
			case 'c':
				ASSIGN(preproc);
compile:
			case 'i':
				ASSIGN(compile);
				goto after_compile;
assemb:
			case 'S':
				file->preproc_asm = 1;
				ASSIGN(preproc); /* preprocess .S assembly files by default */
after_compile:
			case 's':
				ASSIGN(assemb);
				file->out = file->assemb;
				break;

			default:
			assume_obj:
				fprintf(stderr, "assuming \"%s\" is object-file\n", in);
			case 'o':
				/* else assume it's already an object file */
				file->out = file->in;
		}
	}else{
		goto assume_obj;
	}
}

static void gen_obj_file(struct cc_file *file, char **args[4], enum mode mode)
{
	char *in = file->in;

	if(file->preproc){
		/* if we're preprocessing, but not cc1'ing, but we are as'ing,
		 * it's an assembly language file */
		if(file->preproc_asm)
			dynarray_add(&args[mode_preproc], ustrdup("-D__ASSEMBLER__=1"));

		preproc(in, file->preproc, args[mode_preproc]);

		in = file->preproc;
	}

	if(mode == mode_preproc)
		return;

	if(file->compile){
		compile(in, file->compile, args[mode_compile]);

		in = file->compile;
	}

	if(mode == mode_compile)
		return;

	if(file->assemb){
		assemble(in, file->assemb, args[mode_assemb]);
	}
}

static void rename_files(struct cc_file *files, int nfiles, char *output, enum mode mode)
{
	const char mode_ch = MODE_ARG_CH(mode);
	int i;

	for(i = 0; i < nfiles; i++){
		/* path/to/input.c -> path/to/input.[os] */
		char *new;

		if(mode < FILE_IN_MODE(&files[i])){
			fprintf(stderr, "input \"%s\" unused with -%c present\n",
					files[i].in, mode_ch);
			continue;
		}

		if(output){
			if(mode == mode_preproc){
				/* append to current */
				if(files[i].preproc)
					cat(files[i].preproc, output, i);
				continue;
			}else if(mode == mode_compile && !strcmp(output, "-")){
				/* -S -o- */
				if(files[i].compile)
					cat(files[i].compile, NULL, 0);
				continue;
			}

			new = ustrdup(output);
		}else{
			switch(mode){
				case mode_link:
					die("mode_link for multiple files");

				case mode_preproc:
					if(files[i].preproc)
						cat(files[i].preproc, NULL, 0);
					continue;

				case mode_compile:
				case mode_assemb:
				{
					int len;
					new = ustrdup(files[i].in);
					len = strlen(new);
					if(len > 2 && new[len - 2] == '.')
						new[len - 1] = mode == mode_compile ? 's' : 'o';
					/* else stick with what we were given */
					break;
				}
			}
		}

		rename_or_move(files[i].out, new);

		free(new);
	}
}

static void process_files(enum mode mode, char **inputs, char *output, char **args[4], char *backend)
{
	const int ninputs = dynarray_count(inputs);
	int i;
	struct cc_file *files;
	char **links = NULL;

	files = umalloc(ninputs * sizeof *files);

	/* crt must come first */
	if(!gopts.nostartfiles)
		dynarray_add_array(&links, ld_crt_args());


	if(backend){
		dynarray_add(&args[mode_compile], ustrdup("-X"));
		dynarray_add(&args[mode_compile], ustrdup(backend));
	}

	for(i = 0; i < ninputs; i++){
		create_file(&files[i], mode, inputs[i]);

		gen_obj_file(&files[i], args, mode);

		dynarray_add(&links, ustrdup(files[i].out));
	}

	if(mode == mode_link){
		/* An object file's unresolved symbols must
		 * be _later_ in the linker's argv array.
		 * crt, user files, then stdlib
		 */
		if(!gopts.nostdlib)
			/* ld_crt_args() refers to static memory */
			dynarray_add_array(&links, ld_stdlib_args());

		if(!output)
			output = "a.out";

		link_all(links, output, args[mode_link]);

		if(NEED_DSYM && gdebug)
			dsym(output);
	}else{
		rename_files(files, ninputs, output, mode);
	}

	dynarray_free(char **, &links, free);

	/*for(i = 0; i < ninputs; i++)
		free_file(&files[i]);*/

	free(files);
}

ucc_dead
void die(const char *fmt, ...)
{
	const int len = strlen(fmt);
	const int err = len > 0 && fmt[len - 1] == ':';
	va_list l;

	fprintf(stderr, "%s: ", argv0);

	va_start(l, fmt);
	vfprintf(stderr, fmt, l);
	va_end(l);

	if(err)
		fprintf(stderr, " %s", strerror(errno));

	fputc('\n', stderr);

	exit(1);
}

void ice(const char *f, int line, const char *fn, const char *fmt, ...)
{
	va_list l;
	fprintf(stderr, "ICE: %s:%d:%s: ", f, line, fn);
	va_start(l, fmt);
	vfprintf(stderr, fmt, l);
	va_end(l);
	abort();
}

static void add_cfg_args(char ***par, const char *args)
{
	dynarray_add_tmparray(par, strsplit(args, " "));
}

int main(int argc, char **argv)
{
	enum mode mode = mode_link;
	int i, syntax_only = 0;
	int stdinc = 1;
	char **includes = NULL;
	char **inputs = NULL;
	char **args[4] = { 0 };
	char *output = NULL;
	char *backend = NULL;
	struct
	{
		char optn;
		char **ptr;
	} opts[] = {
		{ 'o', &output   },
		{ 'X', &backend  },
	};

	umask(0077); /* prevent reading of the temporary files we create */

	gopts.assume = -1;
	argv0 = argv[0];

	if(argc <= 1){
		fprintf(stderr, "Usage: %s [options] input(s)\n", *argv);
		return 1;
	}

	/* we don't want the initial temporary fname "/tmp/tmp.xyz" tracked
	 * or showing up in error messages
	 */
	dynarray_add(&args[mode_compile], ustrdup("-fno-track-initial-fname"));

	/* bring in CPPFLAGS and CFLAGS */
	add_cfg_args(&args[mode_compile], UCC_CFLAGS);
	add_cfg_args(&args[mode_preproc], UCC_CPPFLAGS);


	for(i = 1; i < argc; i++){
		if(!strcmp(argv[i], "--")){
			while(++i < argc)
				dynarray_add(&inputs, argv[i]);
			break;

		}else if(*argv[i] == '-'){
			int found = 0;
			unsigned int j;
			char *arg = argv[i];

			switch(arg[1]){
#define ADD_ARG(to) dynarray_add(&args[to], ustrdup(arg))

				case 'W':
				{
					/* check for W%c, */
					char c;
					if(sscanf(arg, "-W%c", &c) == 1 && arg[3] == ','){
						switch(c){
#define MAP(c, l) case c: arg += 4; goto arg_ ## l;
							MAP('p', cpp);
							MAP('a', asm);
							MAP('l', ld);
#undef MAP
							default:
								fprintf(stderr, "argument \"%s\" assumed to be for cc1\n", arg);
						}
					}
					/* else default to -Wsomething - add to cc1 */

					/* also add to cpp */
					ADD_ARG(mode_preproc);
				}

				case 'f':
					if(!strcmp(arg, "-fsyntax-only")){
						syntax_only = 1;
						continue;
					}
					else if(!strcmp(argv[i], "-ffreestanding")){
						/* preproc gets this too */
						ADD_ARG(mode_preproc);
					}
					else if(!strncmp(argv[i], "-fmessage-length=", 17)){
						ADD_ARG(mode_preproc);
					}

				case 'w':
					if(argv[i][1] == 'w' && argv[i][2])
						goto word;
				case 'm':
					ADD_ARG(mode_compile);
					continue;

				case 'D':
				case 'U':
					found = 1;
				case 'P':
arg_cpp:
					ADD_ARG(mode_preproc);
					if(found){
						if(!arg[2]){
							/* allow a space, e.g. "-D" "arg" */
							if(!(arg = argv[++i]))
								goto missing_arg;
							ADD_ARG(mode_preproc);
						}
					}
					continue;
				case 'I':
					dynarray_add(&includes, ustrdup(arg));
					continue;

arg_asm:
					ADD_ARG(mode_assemb);
					continue;

				case 'l':
				case 'L':
arg_ld:
					ADD_ARG(mode_link);
					continue;

#define CHECK_1() if(argv[i][2]) goto unrec;
				case 'E': CHECK_1(); mode = mode_preproc; continue;
				case 'S': CHECK_1(); mode = mode_compile; continue;
				case 'c': CHECK_1(); mode = mode_assemb;  continue;

				case 'o':
					if(argv[i][2]){
						output = argv[i] + 2;
					}else{
						output = argv[++i];
						if(!output)
							goto missing_arg;
					}
					continue;

				case 'g':
					/* debug */
					if(argv[i][2])
						die("-g... unexpected");
					ADD_ARG(mode_compile);
					gdebug = 1;
					/* don't pass to the assembler */
					continue;

				case 'M':
				case 'd':
				case 'C': /* -C and -CC */
					goto arg_cpp;

				case 'x':
				{
					const char *arg;
					/* prevent implicit assumption of source */
					if(argv[i][2])
						arg = argv[i] + 2;
					else if(!argv[++i])
						goto missing_arg;
					else
						arg = argv[i];

					/* TODO: "asm-with-cpp"? */
					/* TODO: order-sensitive -x */
					if(!strcmp(arg, "c"))
						gopts.assume = mode_preproc;
					else if(!strcmp(arg, "cpp-output"))
						gopts.assume = mode_compile;
					else if(!strcmp(arg, "asm"))
						gopts.assume = mode_assemb;
					else if(!strcmp(arg, "none"))
						gopts.assume = -1; /* reset */
					else
						die("-x accepts \"c\", \"cpp-output\", \"asm\" "
								"or \"none\", not \"%s\"", arg);
					continue;
				}

				case '\0':
					/* "-" aka stdin */
					goto input;

word:
				default:
					if(!strncmp(argv[i], "-std=", 5) || !strcmp(argv[i], "-ansi")){
						ADD_ARG(mode_compile);
						ADD_ARG(mode_preproc);
					}
					else if(!strcmp(argv[i], "-nostdlib"))
						gopts.nostdlib = 1;
					else if(!strcmp(argv[i], "-nostartfiles"))
						gopts.nostartfiles = 1;
					else if(!strcmp(argv[i], "-nostdinc"))
						stdinc = 0;
					else if(!strcmp(argv[i], "-###"))
						ucc_ext_cmds_show(1), ucc_ext_cmds_noop(1);
					else if(!strcmp(argv[i], "-v"))
						ucc_ext_cmds_show(1);
					else if(!strcmp(argv[i], "-wrapper")){
						/* -wrapper echo,-n etc */
						wrapper = argv[++i];
						if(!wrapper)
							goto missing_arg;
					}
					else if(!strcmp(argv[i], "-trigraphs"))
						ADD_ARG(mode_preproc);
					else if(!strcmp(argv[i], "-digraphs"))
						ADD_ARG(mode_preproc);
					else if(!strcmp(argv[i], "--no-rm"))
						unlink_tmps = 0;
					else
						break;

					continue;
			}

			for(j = 0; j < sizeof(opts) / sizeof(opts[0]); j++)
				if(argv[i][1] == opts[j].optn){
					if(argv[i][2]){
						*opts[j].ptr = argv[i] + 2;
					}else{
						if(!argv[++i])
							goto missing_arg;
						*opts[j].ptr = argv[i];
					}
					found = 1;
					break;
				}

			if(!found){
unrec:	die("unrecognised option \"%s\"", argv[i]);
missing_arg:	die("need argument for %s", argv[i - 1]);
			}
		}else{
input:	dynarray_add(&inputs, argv[i]);
		}
	}

	{
		const int ninputs = dynarray_count(inputs);

		if(output && ninputs > 1 && (mode == mode_compile || mode == mode_assemb))
			die("can't specify '-o' with '-%c' and an output", MODE_ARG_CH(mode));

		if(syntax_only){
			if(output || mode != mode_link)
				die("-%c specified in syntax-only mode",
						output ? 'o' : MODE_ARG_CH(mode));

			mode = mode_compile;
			output = "/dev/null";
		}

		if(ninputs == 0)
			die("no inputs");
	}


	if(output && mode == mode_preproc && !strcmp(output, "-"))
		output = NULL;
	/* other case is -S, which is handled in rename_files */

	if(backend){
		/* -Xprint stops early */
		mode = mode_compile;
		if(!output)
			output = "-";
	}

	/* default include paths */
	if(stdinc)
		add_cfg_args(&args[mode_preproc], UCC_INC);
	/* custom incldue paths */
	if(includes)
		dynarray_add_tmparray(&args[mode_preproc], includes);

	/* got arguments, a mode, and files to link */
	process_files(mode, inputs, output, args, backend);

	for(i = 0; i < 4; i++)
		dynarray_free(char **, &args[i], free);
	dynarray_free(char **, &inputs, NULL);

	return 0;
}
