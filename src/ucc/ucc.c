#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>

/* umask */
#include <sys/types.h>
#include <sys/stat.h>

#include "ucc.h"
#include "ucc_ext.h"
#include "spec.h"
#include "../util/alloc.h"
#include "../util/dynarray.h"
#include "../util/util.h"
#include "../util/platform.h"
#include "../util/tmpfile.h"
#include "../util/str.h"
#include "str.h"
#include "warning.h"

enum mode
{
	mode_preproc,
	mode_compile,
	mode_assemb,
	mode_link
};
#define MODE_ARG_CH(m) ("ESc\0"[m])

#define FILE_UNINIT -2
struct cc_file
{
	struct fd_name_pair
	{
		char *fname;
		int fd;
	} in;

	struct fd_name_pair preproc, compile, assemb;

	struct fd_name_pair out;

	int preproc_asm;
	int assume;
};
#define FILE_IN_MODE(f)        \
((f)->preproc.fname ? mode_preproc : \
 (f)->compile.fname ? mode_compile : \
 (f)->assemb.fname  ? mode_assemb  : \
 mode_link)

struct ucc
{
	/* be sure to update merge_states() */
	char **inputs;
	char **args[4];
	char **includes;
	char *backend;
	const char **isystems;

	int syntax_only;
	enum mode mode;
};

static char **remove_these;
static int unlink_tmps = 1;
const char *argv0;
char *wrapper;
int fsystem_cpp;

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

static void tmpfilenam(struct fd_name_pair *pair)
{
	char *path;
	int fd = tmpfile_prefix_out("ucc.", &path);

	if(fd == -1)
		die("tmpfile(%s):", path);

	if(!remove_these) /* only register once */
		atexit(unlink_files);

	dynarray_add(&remove_these, path);

	pair->fname = path;
	pair->fd = fd;
}

static void create_file(
		struct cc_file *file,
		int assumption,
		enum mode mode,
		char *in)
{
	char *ext;

	file->in.fname = in;
	file->in.fd = FILE_UNINIT;

	switch(assumption){
		case mode_preproc:
			goto preproc;
		case mode_compile:
			goto compile;
		case mode_assemb:
			goto assemb;
		case mode_link:
			goto assume_obj;
	}

#define FILL_WITH_TMP(x)         \
			if(!file->x.fname){        \
				tmpfilenam(&file->x);    \
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
				FILL_WITH_TMP(preproc);
compile:
			case 'i':
				FILL_WITH_TMP(compile);
				goto after_compile;
assemb:
			case 'S':
				file->preproc_asm = 1;
				FILL_WITH_TMP(preproc); /* preprocess .S assembly files by default */
after_compile:
			case 's':
				FILL_WITH_TMP(assemb);
				file->out = file->assemb;
				break;

			default:
			assume_obj:
				fprintf(stderr, "assuming \"%s\" is object-file\n", in);
			case 'o':
			case 'a':
				/* else assume it's already an object file */
				file->out = file->in;
		}
	}else{
		if(!strcmp(in, "-"))
			goto preproc;
		goto assume_obj;
	}
}

static void gen_obj_file(
		struct cc_file *file, char **args[4], enum mode mode, const struct specopts *opts)
{
	char *in = file->in.fname;

	if(file->preproc.fname){
		/* if we're preprocessing, but not cc1'ing, but we are as'ing,
		 * it's an assembly language file */
		if(file->preproc_asm)
			dynarray_add(&args[mode_preproc], ustrdup("-D__ASSEMBLER__=1"));

		preproc(in, file->preproc.fname, args[mode_preproc]);

		in = file->preproc.fname;
	}

	if(mode == mode_preproc)
		return;

	if(file->compile.fname){
		compile(in, file->compile.fname, args[mode_compile]);

		in = file->compile.fname;
	}

	if(mode == mode_compile)
		return;

	if(file->assemb.fname){
		assemble(in, file->assemb.fname, args[mode_assemb], opts->as);
	}
}

static void rename_files(struct cc_file *files, int nfiles, const char *output, enum mode mode)
{
	const char mode_ch = MODE_ARG_CH(mode);
	int i;

	for(i = 0; i < nfiles; i++){
		/* path/to/input.c -> input.[os]
		 * directory names all trimmed */
		char *new;

		if(mode < FILE_IN_MODE(&files[i])){
			fprintf(stderr, "input \"%s\" unused with -%c present\n",
					files[i].in.fname, mode_ch);
			continue;
		}

		if(output){
			if(mode == mode_preproc){
				/* append to current */
				if(files[i].preproc.fname)
					cat(files[i].preproc.fname, output, i);
				continue;
			}else if(mode == mode_compile && !strcmp(output, "-")){
				/* -S -o- */
				if(files[i].compile.fname)
					cat(files[i].compile.fname, NULL, 0);
				continue;
			}

			new = ustrdup(output);
		}else{
			switch(mode){
				case mode_link:
					die("mode_link for multiple files");

				case mode_preproc:
					if(files[i].preproc.fname)
						cat(files[i].preproc.fname, NULL, 0);
					continue;

				case mode_compile:
				case mode_assemb:
				{
					int len;
					const char *base = strrchr(files[i].in.fname, '/');
					if(!base++)
						base = files[i].in.fname;

					new = ustrdup(base);
					len = strlen(new);
					if(len > 2 && new[len - 2] == '.')
						new[len - 1] = mode == mode_compile ? 's' : 'o';
					/* else stick with what we were given */
					break;
				}
			}
		}

		rename_or_move(files[i].out.fname, new);

		free(new);
	}
}

static void process_files(
		enum mode mode,
		char **inputs,
		int *assumptions,
		const char *output,
		char **args[4],
		char *backend,
		const struct specopts *opts)
{
	const int ninputs = dynarray_count(inputs);
	int i;
	struct cc_file *files;
	char **links = NULL;

	files = umalloc(ninputs * sizeof *files);

	dynarray_add_array(&links, ((struct specopts *)opts)->ldflags_pre_user);

	if(backend){
		dynarray_add(&args[mode_compile], ustrprintf("-emit=%s", backend));
	}

	for(i = 0; i < ninputs; i++){
		create_file(&files[i], assumptions[i], mode, inputs[i]);

		gen_obj_file(&files[i], args, mode, opts);

		dynarray_add(&links, ustrdup(files[i].out.fname));
	}

	if(mode == mode_link){
		/* An object file's unresolved symbols must
		 * be _later_ in the linker's argv array.
		 * crt, user files, then stdlib
		 */
		dynarray_add_array(&links, ((struct specopts *)opts)->ldflags_post_user);

		link_all(links, output, args[mode_link], opts->ld);

		if(opts->post_link && *str_spc_skip(opts->post_link)){
			char *exebuf[3];
			exebuf[0] = "-c";
			exebuf[1] = opts->post_link;
			exebuf[2] = NULL;
			execute("sh", exebuf);
		}
	}else{
		rename_files(files, ninputs, output, mode);
	}

	dynarray_free(char **, links, free);

	for(i = 0; i < ninputs; i++){
		close(files[i].in.fd);
		close(files[i].preproc.fd);
		close(files[i].compile.fd);
		close(files[i].assemb.fd);
	}

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

static void pass_warning(char **args[4], const char *arg)
{
	enum warning_owner owner;

	assert(!strncmp(arg, "-W", 2));
	owner = warning_owner(arg + 2);

	switch((int)owner){
		case W_OWNER_CPP:
			dynarray_add(&args[mode_preproc], ustrdup(arg));
			break;
		case W_OWNER_CC1:
		default: /* give to cc1 by default - it has the -Werror=unknown-warning logic */
			dynarray_add(&args[mode_compile], ustrdup(arg));
			break;
		case W_OWNER_CPP | W_OWNER_CC1:
			dynarray_add(&args[mode_preproc], ustrdup(arg));
			dynarray_add(&args[mode_compile], ustrdup(arg));
			break;
	}
}

static void parse_argv(
		int argc, char **argv,
		struct ucc *const state,
		struct specvars *specvars,
		int *const assumptions,
		int *const current_assumption)
{
	int i;

	for(i = 0; i < argc; i++){
		if(!strcmp(argv[i], "--")){
			while(++i < argc)
				dynarray_add(&state->inputs, argv[i]);
			break;

		}else if(*argv[i] == '-'){
			int found = 0;
			char *arg = argv[i];

			switch(arg[1]){
#define ADD_ARG(to) dynarray_add(&state->args[to], ustrdup(arg))

				case 'W':
				{
					/* check for W%c, */
					char c;
					if(sscanf(arg, "-W%c", &c) == 1 && arg[3] == ','){
						/* split by commas */
						char **entries = strsplit(arg + 4, ",");

						switch(c){
#define MAP(c, to) case c: dynarray_add_tmparray(&state->args[to], entries); break
							MAP('p', mode_preproc);
							MAP('c', mode_compile);
							MAP('a', mode_assemb);
							MAP('l', mode_link);
#undef MAP
							default:
								fprintf(stderr, "argument \"%s\" assumed to be for cc1\n", arg);
								dynarray_add_tmparray(&state->args[mode_compile], entries);
						}
						continue;
					}

					/* else pass to cc1 and possibly cpp */
					pass_warning(state->args, arg);
					continue;
				}

				case 'f':
					/* fopts for ucc: */
					if(!strcmp(arg, "-fsyntax-only")){
						state->syntax_only = 1;
						continue;
					}
					if(!strcmp(argv[i], "-fsystem-cpp")){
						fsystem_cpp = 1;
						continue;
					}
					if(!strcmp(argv[i], "-fleading-underscore")
					|| !strcmp(argv[i], "-fno-leading-underscore"))
					{
						const int no = (argv[i][2] == 'n');

						dynarray_add(&state->args[mode_preproc], ustrprintf("-%c__LEADING_UNDERSCORE", no ? 'U' : 'D'));
						dynarray_add(&state->args[mode_compile], ustrdup(argv[i]));
						continue;
					}
					if(!strcmp(argv[i], "-fpic")
					|| !strcmp(argv[i], "-fPIC")
					|| !strcmp(argv[i], "-fno-pic")
					|| !strcmp(argv[i], "-fno-PIC"))
					{
						const int no = (argv[i][2] == 'n');

						if(no){
							dynarray_add(&state->args[mode_preproc], ustrprintf("-U__PIC__"));
							dynarray_add(&state->args[mode_preproc], ustrprintf("-U__pic__"));
						}else{
							int piclevel = (argv[i][2] == 'P' ? 2 : 1);

							dynarray_add(&state->args[mode_preproc], ustrprintf("-D__PIC__=%d", piclevel));
							dynarray_add(&state->args[mode_preproc], ustrprintf("-D__pic__=%d", piclevel));
						}

						dynarray_add(&state->args[mode_compile], ustrdup(argv[i]));
						continue;
					}

					/* pull out some that cpp wants too: */
					if(!strcmp(argv[i], "-ffreestanding")
					|| !strncmp(argv[i], "-fmessage-length=", 17))
					{
						/* preproc gets this too */
						ADD_ARG(mode_preproc);
					}

					/* pass the rest onto cc1 */
					ADD_ARG(mode_compile);
					continue;

				case 'w':
					if(argv[i][2])
						goto word; /* -wabc... */
					ADD_ARG(mode_preproc); /* -w */
				case 'm':
					ADD_ARG(mode_compile);
					if(!strcmp(argv[i] + 2, "32") || !strcmp(argv[i] + 2, "64"))
						ADD_ARG(mode_preproc);
					continue;

				case 'D':
				case 'U':
					found = 1;
				case 'P':
arg_cpp:
					ADD_ARG(mode_preproc);
					if(!strcmp(argv[i] + 1, "MM"))
						state->mode = mode_preproc; /* cc -MM *.c stops after preproc */

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
					if(arg[2]){
						dynarray_add(&state->includes, ustrdup(arg));
					}else{
						if(!argv[++i])
							die("-I needs an argument");

						dynarray_add(&state->includes, ustrprintf("-I%s", argv[i]));
					}
					continue;

				case 'l':
				case 'L':
arg_ld:
					ADD_ARG(mode_link);
					continue;

#define CHECK_1() if(argv[i][2]) goto unrec;
				case 'E': CHECK_1(); state->mode = mode_preproc; continue;
				case 'S': CHECK_1(); state->mode = mode_compile; continue;
				case 'c': CHECK_1(); state->mode = mode_assemb;  continue;

				case 'o':
					if(argv[i][2]){
						specvars->output = argv[i] + 2;
					}else{
						specvars->output = argv[++i];
						if(!specvars->output)
							goto missing_arg;
					}
					continue;

				case 'O':
					ADD_ARG(mode_compile);
					ADD_ARG(mode_preproc); /* __OPTIMIZE__, etc */
					continue;

				case 'g':
					/* debug */
					if(argv[i][2])
						die("-g... unexpected");
					ADD_ARG(mode_compile);
					specvars->debug = 1;
					/* don't pass to the assembler */
					continue;

				case 'M':
				case 'd':
				case 'C': /* -C and -CC */
					goto arg_cpp;

				case 'X':
				{
					/* check for passing arguments */
					int target = -1;

					/**/ if(!strcmp(argv[i] + 2, "assembler"))
						target = mode_assemb;
					else if(!strcmp(argv[i] + 2, "preprocessor"))
						target = mode_preproc;
					else if(!strcmp(argv[i] + 2, "linker"))
						target = mode_link;

					if(target == -1)
						break;

					if(++i == argc)
						goto missing_arg;

					arg = argv[i];
					ADD_ARG(target);
					continue;
				}

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
					if(!strcmp(arg, "c"))
						*current_assumption = mode_preproc;
					else if(!strcmp(arg, "cpp-output"))
						*current_assumption = mode_compile;
					else if(!strcmp(arg, "asm") || !strcmp(arg, "assembler"))
						*current_assumption = mode_assemb;
					else if(!strcmp(arg, "none"))
						*current_assumption = -1; /* reset */
					else
						die("-x accepts \"c\", \"cpp-output\", \"asm\", \"assembler\" "
								"or \"none\", not \"%s\"", arg);
					continue;
				}

				case '\0':
					/* "-" aka stdin */
					goto input;

word:
				default:
					if(!strcmp(argv[i], "-s"))
						goto arg_ld;

					if(!strncmp(argv[i], "-std=", 5) || !strcmp(argv[i], "-ansi")){
						ADD_ARG(mode_compile);
						ADD_ARG(mode_preproc);
					}
					else if(!strcmp(argv[i], "-pedantic") || !strcmp(argv[i], "-pedantic-errors"))
						ADD_ARG(mode_compile);
					else if(!strcmp(argv[i], "-nostdlib"))
						specvars->stdlib = 0;
					else if(!strcmp(argv[i], "-nostartfiles"))
						specvars->startfiles = 0;
					else if(!strcmp(argv[i], "-nostdinc"))
						specvars->stdinc = 0;
					else if(!strcmp(argv[i], "-shared"))
						specvars->shared = 1;
					else if(!strcmp(argv[i], "-static"))
						specvars->static_ = 1;
					else if(!strcmp(argv[i], "-###"))
						ucc_ext_cmds_show(1), ucc_ext_cmds_noop(1);
					else if(!strcmp(argv[i], "-v"))
						ucc_ext_cmds_show(1);
					else if(!strncmp(argv[i], "-emit", 5)){
						switch(argv[i][5]){
							case '=':
								state->backend = argv[i] + 6;
								break;
							case '\0':
								if(++i == argc)
									goto missing_arg;

								state->backend = argv[i];
								break;
							default:
								goto unrec;
						}
					}
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
					else if(!strcmp(argv[i], "-save-temps"))
						unlink_tmps = 0;
					else if(!strcmp(argv[i], "-isystem")){
						const char *sysinc = argv[++i];
						if(!sysinc)
							goto missing_arg;
						dynarray_add(&state->isystems, sysinc);
					}
					else
						break;

					continue;
			}

unrec:
			die("unrecognised option \"%s\"", argv[i]);
missing_arg:
			die("need argument for %s", argv[i - 1]);
		}else{
			size_t n;
input:
			n = dynarray_count(state->inputs);
			dynarray_add(&state->inputs, argv[i]);
			assumptions[n] = *current_assumption;
		}
	}
}

static void init_spec(
	struct specopts *specopts,
	const struct specvars *specvars)
{
	char *specpath = actual_path("../", "ucc.spec");
	FILE *f = fopen(specpath, "r");

	if(!f){
		fprintf(stderr, "couldn't open \"%s\": %s\n", specpath, strerror(errno));
	}else{
		struct specerr err = { 0 };

		spec_parse(specopts, specvars, f, &err);

		fclose(f);

		if(err.errstr){
			fprintf(stderr, "%s:%u: spec error: %s\n",
					specpath, err.errline, err.errstr);
		}
	}

	free(specpath);
}

static void merge_states(struct ucc *state, struct ucc *append)
{
	int i;
	dynarray_add_tmparray(&state->inputs, append->inputs);

	for(i = 0; i < 4; i++)
		dynarray_add_tmparray(&state->args[i], append->args[i]);

	dynarray_add_tmparray(&state->includes, append->includes);

	assert(!state->backend);
	state->backend = append->backend;

	dynarray_add_tmparray(&state->isystems, append->isystems);

	assert(!state->syntax_only);

	state->mode = append->mode;
}

int main(int argc, char **argv)
{
	int i;
	struct ucc state = { 0 };
	struct ucc argstate = { 0 };
	struct specopts specopts = { 0 };
	struct specvars specvars = { 0 };
	int *assumptions;
	int current_assumption;

	argstate.mode = mode_link;
	current_assumption = -1;
	assumptions = umalloc((argc - 1) * sizeof(*assumptions));

	specvars.shared = 0;
	specvars.stdinc = 1;
	specvars.stdlib = 1;
	specvars.startfiles = 1;

	umask(0077); /* prevent reading of the temporary files we create */

	argv0 = argv[0];

	if(argc <= 1){
usage:
		fprintf(stderr, "Usage: %s [options] input(s)\n", *argv);
		return 1;
	}

	/* we don't want the initial temporary fname "/tmp/tmp.xyz" tracked
	 * or showing up in error messages */
	dynarray_add(&state.args[mode_compile], ustrdup("-fno-track-initial-fname"));

	/* we must parse argv first for things like -nostdinc.
	 * then we can parse the spec file, then we need to
	 * append argv's inputs, etc onto the state from the spec file */
	parse_argv(argc - 1, argv + 1, &argstate, &specvars, assumptions, &current_assumption);

	if(!specvars.output && argstate.mode == mode_link)
		specvars.output = "a.out";

	init_spec(&specopts, &specvars);

	parse_argv(
			dynarray_count(specopts.initflags),
			specopts.initflags,
			&state,
			&specvars,
			assumptions,
			&current_assumption);

	/* ensure argument state is appended to (spec)state,
	 * allowing it to override things like initflags */
	merge_states(&state, &argstate);

	{
		const int ninputs = dynarray_count(state.inputs);

		if(specvars.output && ninputs > 1 && (state.mode == mode_compile || state.mode == mode_assemb))
			die("can't specify '-o' with '-%c' and an output", MODE_ARG_CH(state.mode));

		if(state.syntax_only){
			if(specvars.output || state.mode != mode_link)
				die("-%c specified in syntax-only mode",
						specvars.output ? 'o' : MODE_ARG_CH(state.mode));

			state.mode = mode_compile;
			specvars.output = "/dev/null";
		}

		if(ninputs == 0)
			goto usage;
	}


	if(specvars.output && state.mode == mode_preproc && !strcmp(specvars.output, "-"))
		specvars.output = NULL;
	/* other case is -S, which is handled in rename_files */

	if(state.backend){
		/* -emit=... stops early */
		state.mode = mode_compile;
		if(!specvars.output)
			specvars.output = "-";
	}

	if(state.isystems){
		const char **i;
		for(i = state.isystems; *i; i++){
			/* cpp doesn't care if it's system or not */
			dynarray_add(&state.args[mode_preproc], ustrprintf("-I%s", *i));
			/* cc1 only wants system - can use -I too */
			dynarray_add(&state.args[mode_compile], ustrprintf("-I%s", *i));
		}

		dynarray_free(const char **, state.isystems, NULL);
	}

	/* custom include paths */
	if(state.includes)
		dynarray_add_tmparray(&state.args[mode_preproc], state.includes);

	/* got arguments, a mode, and files to link */
	process_files(
			state.mode,
			state.inputs,
			assumptions,
			specvars.output,
			state.args,
			state.backend,
			&specopts);

	for(i = 0; i < 4; i++)
		dynarray_free(char **, state.args[i], free);
	dynarray_free(char **, state.inputs, NULL);
	free(assumptions);

	return 0;
}
