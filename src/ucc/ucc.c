#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>

#include "ucc.h"
#include "ucc_ext.h"
#include "ucc_path.h"
#include "umask.h"
#include "../util/alloc.h"
#include "../util/dynarray.h"
#include "../util/util.h"
#include "../util/platform.h"
#include "../util/tmpfile.h"
#include "../util/str.h"
#include "../util/triple.h"
#include "../util/macros.h"
#include "../util/tristate.h"
#include "str.h"
#include "warning.h"
#include "filemodes.h"

#define LINUX_LIBC_PREFIX "/usr/lib/"

#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)

enum mode
{
#define X(mode, desc, suffix) mode_##mode,
#define ALIAS(...)
	FILEMODES
#undef X
#undef ALIAS
		mode_link
};
#define MODE_ARG_CH(m) ("ESc\0"[m])

#define FILE_UNINIT -2
struct cc_file
{
	struct fd_name_pair
	{
		char *fname;
		int fd; /* used to generate and hold onto a temp-file-name */
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
	char **args[5];
	char **includes;
	char *backend;
	const char **isystems;

	int syntax_only;
	enum mode mode;

	const char *as, *ld;
	char **ldflags_pre_user, **ldflags_post_user;
	const char *post_link;

	struct
	{
		/* optimisations */
		enum { SSP_NONE, SSP_ALL, SSP_NORMAL } ssp;
	} spanning_fopt;
};

enum dyld
{
	DYLD_DEFAULT,
	DYLD_GLIBC,
	DYLD_MUSL
};

struct uccvars
{
	const char *target;
	const char *output;

	int static_, shared, rdynamic;
	int stdlibinc, builtininc, defaultlibs, startfiles;
	int debug, profile;
	int pthread;
	enum tristate pie;
	enum tristate multilib;
	enum dyld dyld;
	int help, dumpmachine;

	struct ld_zoptions
	{
		int text;
	} ld_z;
};

static char **remove_these;
static int save_temps = 0;
const char *argv0;
char *wrapper;
char *Bprefix;
const char *binpath_cpp;
mode_t orig_umask = 022;

static void unlink_files(void)
{
	int i;
	for(i = 0; remove_these[i]; i++){
		remove(remove_these[i]);
		free(remove_these[i]);
	}
	free(remove_these);
}

static char *expected_filename(const char *in, enum mode mode)
{
	const char *base = strrchr(in, '/');
	size_t len;
	char *new;

	if(!base++)
		base = in;

	new = ustrdup(base);
	len = strlen(new);
	if(len > 2 && new[len - 2] == '.'){
		char ext;
		switch(mode){
#define X(mode, desc, suffix) case mode_##mode: ext = suffix; break;
#define ALIAS(...)
			FILEMODES
#undef X
#undef ALIAS
			case mode_link:
				assert(0 && "unreachable");
		}

		new[len - 1] = ext;
	}
	/* else stick with what we were given */

	return new;
}

static void tmpfilenam(
		struct fd_name_pair *pair,
		enum mode const mode,
		const char *in)
{
	char *path;
	int fd;

	if(save_temps){
		/* this ignores any directories, e.g.
		 * ucc -save-temps path/to/a.c
		 * will generate ./a.[iso], not path/to/a.[iso]
		 * (like gcc and clang)
		 */
		path = expected_filename(in, mode);
		fd = FILE_UNINIT;
		/* we don't create the temp files for -save-temps because there's no need
		 * to create and hold onto some temporary file - we have the fixed file
		 * output name already */
	}else{
		fd = tmpfile_prefix_out("ucc.", &path);

		if(fd == -1)
			die("tmpfile(%s):", path);

		if(!remove_these) /* only register once */
			atexit(unlink_files);
		dynarray_add(&remove_these, path);
	}

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
		case mode_assemb_with_cpp:
			goto assemb_with_cpp;
		case mode_link:
			goto assume_obj;
	}

#define FILL_WITH_TMP(x)         \
			if(!file->x.fname){        \
				tmpfilenam(&file->x, mode_##x, in); \
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
			case 'S':
assemb_with_cpp:
				file->preproc_asm = 1;
				FILL_WITH_TMP(preproc); /* preprocess .S assembly files by default */
assemb:
after_compile:
			case 's':
				FILL_WITH_TMP(assemb);
				file->out = file->assemb;
				break;

			default:
			assume_obj:
				fprintf(stderr, "assuming \"%s\" is object-file\n", in);
			is_obj:
			case 'o':
			case 'a':
				/* else assume it's already an object file */
				file->out = file->in;
		}
	}else{
		if(!strcmp(in, "-"))
			goto preproc;
		if(ext && !strcmp(ext, ".so"))
			goto is_obj;
		goto assume_obj;
	}
}

static void gen_obj_file(
		struct cc_file *file, char **args[], enum mode mode, const char *as)
{
	char *in = file->in.fname;

	if(file->preproc.fname){
		/* if we're preprocessing, but not cc1'ing, but we are as'ing,
		 * it's an assembly language file */
		if(file->preproc_asm)
			dynarray_add(&args[mode_preproc], ustrdup("-D__ASSEMBLER__=1"));

		preproc(in, file->preproc.fname, args[mode_preproc], 0);

		in = file->preproc.fname;
	}

	if(mode == mode_preproc)
		return;

	if(file->compile.fname){
		compile(in, file->compile.fname, args[mode_compile], 0);

		in = file->compile.fname;
	}

	if(mode == mode_compile)
		return;

	if(file->assemb.fname){
		assemble(in, file->assemb.fname, args[mode_assemb], as);
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
					cat_fnames(files[i].preproc.fname, output, i);
				continue;
			}else if(mode == mode_compile && !strcmp(output, "-")){
				/* -S -o- */
				if(files[i].compile.fname)
					cat_fnames(files[i].compile.fname, NULL, 0);
				continue;
			}

			new = ustrdup(output);
		}else{
			switch(mode){
				case mode_link:
					die("mode_link for multiple files");

				case mode_preproc:
					if(files[i].preproc.fname)
						cat_fnames(files[i].preproc.fname, NULL, 0);
					continue;

				case mode_compile:
				case mode_assemb:
				case mode_assemb_with_cpp:
					new = expected_filename(files[i].in.fname, mode);
					break;
			}
		}

		rename_or_move(files[i].out.fname, new);

		free(new);
	}
}

static void fd_name_close(struct fd_name_pair *f)
{
	if(f->fd < 0)
		return;
	close(f->fd);
	f->fd = FILE_UNINIT;
}

static void process_files(
		struct ucc *state,
		int *assumptions,
		const char *output)
{
	const int ninputs = dynarray_count(state->inputs);
	int i;
	struct cc_file *files;
	char **links = NULL;

	files = umalloc(ninputs * sizeof *files);

	dynarray_add_array(&links, state->ldflags_pre_user);
	dynarray_free(char **, state->ldflags_pre_user, NULL);

	if(state->backend){
		dynarray_add(&state->args[mode_compile], ustrprintf("-emit=%s", state->backend));
	}

	for(i = 0; i < ninputs; i++){
		create_file(&files[i], assumptions[i], state->mode, state->inputs[i]);

		gen_obj_file(&files[i], state->args, state->mode, state->as);

		dynarray_add(&links, ustrdup(files[i].out.fname));
	}

	if(state->mode == mode_link){
		/* An object file's unresolved symbols must
		 * be _later_ in the linker's argv array.
		 * crt, user files, then stdlib
		 */
		dynarray_add_array(&links, state->ldflags_post_user);
		dynarray_free(char **, state->ldflags_post_user, NULL);

		link_all(links, output, state->args[mode_link], state->ld);

		if(state->post_link && *str_spc_skip(state->post_link)){
			char *exebuf[3];
			exebuf[0] = "-c";
			exebuf[1] = (char *)state->post_link;
			exebuf[2] = NULL;
			execute("sh", exebuf);
		}
	}else{
		rename_files(files, ninputs, output, state->mode);
	}

	dynarray_free(char **, links, free);

	for(i = 0; i < ninputs; i++){
		fd_name_close(&files[i].in);
		fd_name_close(&files[i].preproc);
		fd_name_close(&files[i].compile);
		fd_name_close(&files[i].assemb);
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

static void pass_warning(char **args[], const char *arg)
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

static char *generate_depfile(struct ucc *const state, const char *fromflag)
{
	const char *in;
	char *buf;
	char *dot;

	if(dynarray_count(state->inputs) != 1)
		die("cannot specify %s and multiple inputs", fromflag);
	in = strrchr(state->inputs[0], '/');
	if(in)
		in++;
	else
		in = state->inputs[0];

	dot = strrchr(in, '.');
	if(dot){
		const size_t i = dot - in;

		/* +2 in case in="abc.", so we have room for the ext */
		buf = umalloc(strlen(in) + 2);

		strcpy(buf, in);
		buf[i + 1] = 'd';
		buf[i + 2] = '\0';
	}else{
		buf = ustrprintf("%s.d", in);
	}

	return buf;
}

static void remove_macro(struct ucc *const state, const char *a)
{
	dynarray_add(&state->args[mode_preproc], ustrprintf("-U%s", a));
}

static int handle_spanning_fopt(const char *fopt, struct ucc *const state)
{
	const char *name;
	int no = 0;
	int is_pie = 0;

	assert(!strncmp(fopt, "-f", 2));
	name = fopt + 2;
	if(!strncmp(name, "no-", 3)){
		no = 1;
		name += 3;
	}

	if(!strcmp(name, "leading-underscore")){
		dynarray_add(&state->args[mode_preproc], ustrprintf("-%c__LEADING_UNDERSCORE", no ? 'U' : 'D'));
		dynarray_add(&state->args[mode_compile], ustrdup(fopt));

		dynarray_add(
				&state->args[mode_preproc],
				ustrprintf(
					"-%c__USER_LABEL_PREFIX__%s",
					no ? 'U' : 'D',
					no ? "" : "=_"));
		return 1;
	}

	if(!strcmp(name, "pic") || !strcmp(name, "PIC")
	|| (is_pie = !strcmp(name, "pie") || !strcmp(name, "PIE")))
	{
		if(no){
			remove_macro(state, "__PIC__");
			remove_macro(state, "__pic__");
			remove_macro(state, "__PIE__");
			remove_macro(state, "__pie__");
		}else{
			int piclevel = (name[0] == 'P' ? 2 : 1);

			dynarray_add(&state->args[mode_preproc], ustrprintf("-D__PIC__=%d", piclevel));
			dynarray_add(&state->args[mode_preproc], ustrprintf("-D__pic__=%d", piclevel));

			if(is_pie){
				dynarray_add(&state->args[mode_preproc], ustrprintf("-D__PIE__=%d", piclevel));
				dynarray_add(&state->args[mode_preproc], ustrprintf("-D__pie__=%d", piclevel));
			}
		}

		dynarray_add(&state->args[mode_compile], ustrdup(fopt));
		return 1;
	}

	if(!strcmp(name, "signed-char") || !strcmp(name, "unsigned-char")){
		const int is_signed = (fopt[2] == 's' || fopt[5] == 'u');

		dynarray_add(&state->args[mode_preproc], ustrprintf(
					"-%c__CHAR_UNSIGNED__%s",
					is_signed ? 'U' : 'D',
					is_signed ? "" : "=1"));

		dynarray_add(&state->args[mode_compile], ustrdup(fopt));
		return 1;
	}

	if(!strcmp(name, "stack-protector")){
		state->spanning_fopt.ssp = no ? SSP_NONE : SSP_NORMAL;
		return 1;
	}
	if(!strcmp(name, "stack-protector-all")){
		state->spanning_fopt.ssp = no ? SSP_NONE : SSP_ALL;
		return 1;
	}

	if(!strcmp(name, "fast-math")){
		dynarray_add(&state->args[mode_preproc], ustrprintf("-%c__FAST_MATH__", no ? 'U' : 'D'));
		dynarray_add(&state->args[mode_compile], ustrdup(fopt));
		return 1;
	}

	return 0;
}

static void resolve_spanning_fopts(struct ucc *const state)
{
	switch(state->spanning_fopt.ssp){
		case SSP_NONE:
			break;
		case SSP_NORMAL:
			dynarray_add(&state->args[mode_preproc], ustrdup("-D__SSP__=1"));
			dynarray_add(&state->args[mode_compile], ustrdup("-fstack-protector"));
			break;
		case SSP_ALL:
			dynarray_add(&state->args[mode_preproc], ustrdup("-D__SSP_ALL__=2"));
			dynarray_add(&state->args[mode_compile], ustrdup("-fstack-protector-all"));
			break;
	}
}

static int add_normalised_arg(
		struct ucc *const state,
		enum mode mode,
		char **argv,
		int *const pi)
{
	const char *arg = argv[*pi];

	if(arg[2]){
		dynarray_add(&state->args[mode], ustrdup(arg));

	}else{
		char *joined;
		int i;

		/* allow a space, e.g. "-D" "arg" */
		if(!(arg = argv[++*pi]))
			return 0;

		i = *pi;
		joined = ustrprintf("%s%s", argv[i - 1], argv[i]);
		dynarray_add(&state->args[mode], joined);
	}

	return 1;
}

static void parse_argv(
		int argc,
		char **argv,
		struct ucc *const state,
		struct uccvars *vars,
		int *const assumptions,
		int *const current_assumption)
{
#define ADD_ARG(to, arg) dynarray_add(&state->args[to], ustrdup(arg))
	int had_MD = 0, had_MF = 0;
	int i;
	int seen_double_dash = 0;

	for(i = 0; i < argc; i++){
		if(seen_double_dash || !strcmp(argv[i], "--")){
			seen_double_dash = 1;
			goto input;

		}else if(*argv[i] == '-'){
			char *arg = argv[i];

			switch(arg[1]){
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
					if(!strncmp(argv[i], "-fuse-cpp", 9)){
						const char *arg = argv[i] + 9;
						switch(*arg){
							case '\0':
								binpath_cpp = "cpp";
								break;
							case '=':
								binpath_cpp = arg + 1;
								break;
							default:
								die("-fuse-cpp should have no argument, or \"=path/to/cpp\"");
						}
						continue;
					}
					if(handle_spanning_fopt(argv[i], state))
						continue;

					/* pull out some that cpp wants too: */
					if(!strcmp(argv[i], "-ffreestanding")
					|| !strncmp(argv[i], "-fmessage-length=", 17))
					{
						/* preproc gets this too */
						ADD_ARG(mode_preproc, arg);
					}

					if(!strcmp(argv[i], "-fcpp-offsetof")){
						ADD_ARG(mode_preproc, arg);
						continue;
					}

					if(!strncmp(argv[i], "-fsystem-cpp", 12))
						die("-fsystem-cpp has been removed, use -fuse-cpp[=path/to/cpp] instead");

					/* pass the rest onto cc1 */
					ADD_ARG(mode_compile, arg);
					continue;

				case 'w':
					if(argv[i][2])
						goto word; /* -wabc... */
					ADD_ARG(mode_preproc, arg); /* -w */
					ADD_ARG(mode_compile, arg);
					ADD_ARG(mode_assemb, "-W");
					continue;

				case 'm':
				{
					const char *mopt = argv[i] + 2;

					if(!strcmp(mopt, "multilib") || !strcmp(mopt, "no-multilib")){
						vars->multilib = *mopt == 'n' ? TRI_FALSE : TRI_TRUE;
						continue;
					}
					if(!strcmp(mopt, "musl") || !strcmp(mopt, "glibc")){
						vars->dyld = *mopt == 'm' ? DYLD_MUSL : DYLD_GLIBC;
						continue;
					}

					ADD_ARG(mode_compile, arg);
					continue;
				}

				case 'D':
				case 'U':
					if(!add_normalised_arg(state, mode_preproc, argv, &i))
						goto missing_arg;
					continue;

				case 'H':
				case 'P':
arg_cpp:
					ADD_ARG(mode_preproc, arg);
					if(!strcmp(argv[i] + 1, "M")){
						/* cc -M *.c implies -E -w */
						state->mode = mode_preproc;
						dynarray_add(&state->args[mode_preproc], ustrdup("-w"));
						dynarray_add(&state->args[mode_compile], ustrdup("-w"));
					}
					if(!strcmp(argv[i] + 1, "MM"))
						state->mode = mode_preproc; /* cc -MM *.c stops after preproc */
					if(!strcmp(argv[i] + 1, "MD")){
						/* like -M -MF ..., but without implied -E */
						dynarray_add(&state->args[mode_preproc], ustrdup("-w"));
						dynarray_add(&state->args[mode_compile], ustrdup("-w"));

						had_MD = 1;
					}
					if(!strcmp(argv[i] + 1, "MF")){
						i++;
						arg = argv[i];
						if(!arg)
							die("-MF needs an argument");
						ADD_ARG(mode_preproc, arg);
						had_MF = 1;
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
					if(!add_normalised_arg(state, mode_link, argv, &i))
						goto missing_arg;
					continue;

#define CHECK_1() if(argv[i][2]) goto unrec;
				case 'E': CHECK_1(); state->mode = mode_preproc; continue;
				case 'S': CHECK_1(); state->mode = mode_compile; continue;
				case 'c': CHECK_1(); state->mode = mode_assemb;  continue;

				case 'o':
					if(argv[i][2]){
						vars->output = argv[i] + 2;
					}else{
						vars->output = argv[++i];
						if(!vars->output)
							goto missing_arg;
					}
					continue;

				case 'O':
					ADD_ARG(mode_compile, arg);
					ADD_ARG(mode_preproc, arg); /* __OPTIMIZE__, etc */
					continue;

				case 'g':
				{
					/* debug */
					const char *debugopt = argv[i] + 2;

					if(!strcmp(strncmp(debugopt, "no-", 3) ? debugopt : debugopt + 3, "column-info")){
						/* doesn't affect debug output */
					}else if(!strcmp(argv[i], "-g0")){
						vars->debug = 0;
					}else{
						/* some debug option - we're generating debug code */
						vars->debug = 1;
					}

					ADD_ARG(mode_compile, arg);
					/* don't pass to the assembler */
					continue;
				}

				case 'd':
					if(!strcmp(argv[i], "-dumpmachine")){
						vars->dumpmachine = 1;
						continue;
					}
				case 'M':
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
					ADD_ARG(target, arg);
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

#define X(mode, desc, suffix) else if(!strcmp(arg, desc)) *current_assumption = mode_##mode;
#define ALIAS(mode, desc) X(mode, desc, 0)
					if(0);
					FILEMODES
#undef X
#undef ALIAS
					else if(!strcmp(arg, "none"))
						*current_assumption = -1; /* reset */
					else{
#define X(mode, desc, suffix) #desc ", "
#define ALIAS(mode, desc) X(mode, desc, 0)
						die("-x accepts "
								FILEMODES
								"or \"none\", not \"%s\"", arg);
#undef X
#undef ALIAS
					}
					continue;
				}

				case 'B':
				{
					size_t l;
					if(Bprefix) /* not exactly gcc/clang compatible */
						die("-Bprefix already given");

					Bprefix = argv[i] + 2;
					l = strlen(Bprefix);
					if(l == 0)
						die("need argument for -Bprefix");
					if(Bprefix[l - 1] != '/')
						Bprefix = ustrprintf("%s/", Bprefix); /* static data, no leak */
					continue;
				}

				case '\0':
					/* "-" aka stdin */
					goto input;

word:
				default:
					if(!strcmp(argv[i], "-s")){
						ADD_ARG(mode_link, arg);
					}
					else if(!strncmp(argv[i], "-std=", 5) || !strcmp(argv[i], "-ansi")){
						ADD_ARG(mode_compile, arg);
						ADD_ARG(mode_preproc, arg);
					}
					else if(!strcmp(argv[i], "-pedantic") || !strcmp(argv[i], "-pedantic-errors"))
						ADD_ARG(mode_compile, arg);

					/* stdlib = startfiles and defaultlibs */
					else if(!strcmp(argv[i], "-nostdlib"))
						vars->startfiles = 0, vars->defaultlibs = 0;
					else if(!strcmp(argv[i], "-nostartfiles"))
						vars->startfiles = 0;
					else if(!strcmp(argv[i], "-nodefaultlibs"))
						vars->defaultlibs = 0;

					/* stdinc = stdlibinc and builtininc */
					else if(!strcmp(argv[i], "-nostdinc"))
						vars->stdlibinc = 0, vars->builtininc = 0;
					else if(!strcmp(argv[i], "-nostdlibinc"))
						vars->stdlibinc = 0;
					else if(!strcmp(argv[i], "-nobuiltininc"))
						vars->builtininc = 0;

					else if(!strcmp(argv[i], "-shared"))
						vars->shared = 1;
					else if(!strcmp(argv[i], "-static"))
						vars->static_ = 1;
					else if(!strcmp(argv[i], "-rdynamic"))
						vars->rdynamic = 1;
					else if(!strcmp(argv[i], "-pie"))
						vars->pie = TRI_TRUE;
					else if(!strcmp(argv[i], "-no-pie"))
						vars->pie = TRI_FALSE;
					else if(!strcmp(argv[i], "-static-pie")){
						vars->static_ = 1;
						vars->pie = TRI_TRUE;
						vars->ld_z.text = 1; /* disallow text-relocs */
					}
					else if(!strcmp(argv[i], "-pthread"))
						vars->pthread = 1;
					else if(!strcmp(argv[i], "-###"))
						ucc_ext_cmds_show(1), ucc_ext_cmds_noop(1);
					else if(!strcmp(argv[i], "-v"))
						ucc_ext_cmds_show(1);
					else if(!strcmp(argv[i], "-pg")){
						ADD_ARG(mode_compile, arg);
						vars->profile = 1;
					}
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
						ADD_ARG(mode_preproc, arg);
					else if(!strcmp(argv[i], "-digraphs"))
						ADD_ARG(mode_preproc, arg);
					else if(!strcmp(argv[i], "-save-temps"))
						save_temps = 1;
					else if(!strcmp(argv[i], "-isystem")){
						const char *sysinc = argv[++i];
						if(!sysinc)
							goto missing_arg;
						dynarray_add(&state->isystems, sysinc);
					}
					else if(!strcmp(argv[i], "-target")){
						vars->target = argv[i + 1];
						if(!vars->target)
							goto missing_arg;

						ADD_ARG(mode_compile, arg);
						ADD_ARG(mode_preproc, arg);
						arg = argv[++i];
						ADD_ARG(mode_compile, arg);
						ADD_ARG(mode_preproc, arg);
					}
					else if(!strcmp(argv[i], "-time"))
						time_subcmds = 1;
					else
						break;

					continue;
			}

			if(!strcmp(argv[i], "--help")){
				vars->help = 1;
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

	resolve_spanning_fopts(state);

	if(had_MD && !had_MF){
		char *depfile;

		if(vars->output){
			if(state->mode == mode_preproc){
				depfile = ustrdup(vars->output);
			}else{
				depfile = ustrprintf("%s.d", vars->output);
			}
		}else{
			depfile = generate_depfile(state, "-MD");
		}

		dynarray_add(&state->args[mode_preproc], ustrdup("-MF"));
		dynarray_add(&state->args[mode_preproc], depfile);
	}
#undef ADD_ARG
}

static void merge_states(struct ucc *state, struct ucc *append)
{
	size_t i;
	dynarray_add_tmparray(&state->inputs, append->inputs);

	for(i = 0; i < countof(state->args); i++)
		dynarray_add_tmparray(&state->args[i], append->args[i]);

	dynarray_add_tmparray(&state->includes, append->includes);

	assert(!state->backend);
	state->backend = append->backend;

	dynarray_add_tmparray(&state->isystems, append->isystems);

	assert(!state->syntax_only);
	state->syntax_only = append->syntax_only;

	state->mode = append->mode;

	if(!state->as)
		state->as = "as";
	if(!state->ld)
		state->ld = "ld";
}

static void vars_default(struct uccvars *vars)
{
	vars->stdlibinc = 1;
	vars->builtininc = 1;
	vars->defaultlibs = 1;
	vars->startfiles = 1;
	vars->pie = TRI_UNSET;
	vars->multilib = TRI_UNSET;
}

static int should_multilib(enum tristate multilib, const char *prefix)
{
	/*
	 * decide whether we're on a multilib system
	 * multilib: /usr/lib/x86_64-linux-gnu/crt1.o
	 * normal:   /usr/lib/crt1.o
	 */
	char path[64];

	switch(multilib){
		case TRI_FALSE: return 0;
		case TRI_TRUE: return 1;
		case TRI_UNSET: break;
	}

	xsnprintf(path, sizeof(path), LINUX_LIBC_PREFIX "%s", prefix);

	/* note that this ignores cross compiling
	 * gcc and clang have this as a build-time option
	 */
	return access(path, F_OK) == 0;
}

static const char *darwin_syslibroot(int *const alloc)
{
	/* /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/xcselect.h */
	enum { XCSELECT_HOST_SDK_POLICY_MATCHING_PREFERRED = 1 };

	static const char *const roots[] = {
		"/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk",
		"/Applications/Xcode.app/Developer/SDKs/MacOSX.sdk",
		"/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk",
		NULL
	};
	const char *const *i;
	const char *syslibroot = NULL;
	char *sdk_path;
	void *dl;
	typedef int xcselect_host_sdk_path_ty(unsigned, char **);
	xcselect_host_sdk_path_ty *xcselect_host_sdk_path;

	*alloc = 0;
	for(i = roots; *i; i++)
		if(access(*i, F_OK) == 0)
			return *i;

	dl = dlopen("libxcselect.dylib", RTLD_GLOBAL | RTLD_LAZY);
	if(!dl)
		return NULL;

	xcselect_host_sdk_path = (xcselect_host_sdk_path_ty *)dlsym(dl, "xcselect_host_sdk_path");
	if(!xcselect_host_sdk_path)
		return NULL;

	if(xcselect_host_sdk_path(XCSELECT_HOST_SDK_POLICY_MATCHING_PREFERRED, &sdk_path) != 0)
		return NULL;

	*alloc = 1;
	return sdk_path;
}

static void state_from_triple(
		struct ucc *state,
		char ***additional_argv,
		const struct uccvars *vars,
		const struct triple *triple)
{
	const char *paramshared = "-shared";
	const char *paramstatic = "-static";

	if(vars->builtininc){
		struct cmdpath uccinc;
		cmdpath_initrelative(&uccinc, "include", "../include");

		dynarray_add(&state->args[mode_preproc], ustrdup("-isystem"));
		dynarray_add(&state->args[mode_preproc], cmdpath_resolve(&uccinc, NULL));
	}
	if(vars->stdlibinc){
		dynarray_add(&state->args[mode_preproc], ustrdup("-isystem"));
		dynarray_add(&state->args[mode_preproc], ustrdup("/usr/include"));

		dynarray_add(&state->args[mode_preproc], ustrdup("-isystem"));
		dynarray_add(&state->args[mode_preproc], ustrdup("/usr/local/include"));
	}

	if(triple->sys != SYS_linux && vars->dyld != DYLD_DEFAULT)
		die("-mmusl/-mglibc given for non-linux system");

	if(triple->sys != SYS_linux && triple->sys != SYS_darwin && vars->rdynamic)
		die("-rdynamic given for non-linux/darwin system");

	switch(triple->sys){
		case SYS_linux:
		{
			const char *const target = triple_to_str(triple, 0);
			const char *multilib_prefix = target;
			const int is_pie = vars->pie != TRI_FALSE;

			if(!should_multilib(vars->multilib, multilib_prefix))
				multilib_prefix = "";

			if(is_pie && !vars->shared)
				dynarray_add(&state->ldflags_pre_user, ustrdup("-pie"));

			if(vars->rdynamic)
				dynarray_add(&state->ldflags_pre_user, ustrdup("-export-dynamic"));

			if(vars->shared){
				/* don't mention a dynamic linker - not used for generating a shared library */
			}else{
				if(!vars->static_){
					const char *dyld;

					dynarray_add(&state->ldflags_pre_user, ustrdup("-dynamic-linker"));

					switch(vars->dyld){
						case DYLD_DEFAULT:
						case DYLD_GLIBC:
							dyld = "/lib64/ld-linux-x86-64.so.2";
							break;
						case DYLD_MUSL:
							dyld = "/lib/ld-musl-x86_64.so.1";
							break;
					}
					dynarray_add(&state->ldflags_pre_user, ustrdup(dyld));

				}else{
					dynarray_add(&state->ldflags_pre_user, ustrdup("-no-dynamic-linker"));
				}
			}

			dynarray_add(&state->ldflags_post_user, ustrdup("-L" LINUX_LIBC_PREFIX));

			if(vars->defaultlibs){
				dynarray_add(&state->ldflags_post_user, ustrdup("-lc"));
			}

			if(vars->startfiles){
				char usrlib[64];

				if(vars->shared){
					/* don't link to crt1 - don't want the startup files, just i[nit] and e[nd] */
				}else{
					if(vars->profile){
						xsnprintf(usrlib, sizeof(usrlib), LINUX_LIBC_PREFIX "%s/gcrt1.o", multilib_prefix);
					}else if(is_pie){
						if(vars->static_)
							xsnprintf(usrlib, sizeof(usrlib), LINUX_LIBC_PREFIX "%s/rcrt1.o", multilib_prefix);
						else
							xsnprintf(usrlib, sizeof(usrlib), LINUX_LIBC_PREFIX "%s/Scrt1.o", multilib_prefix);
					}else{
						xsnprintf(usrlib, sizeof(usrlib), LINUX_LIBC_PREFIX "%s/crt1.o", multilib_prefix);
					}
					dynarray_add(&state->ldflags_pre_user, ustrdup(usrlib));
				}

				{
					struct cmdpath dso;
					char *resolved;

					cmdpath_initrelative(&dso, "../rt/dsohandle.o", "../rt/dsohandle.o");
					resolved = cmdpath_resolve(&dso, NULL);

					dynarray_add(&state->ldflags_pre_user, resolved);
				}

				{
					char *dot;

					xsnprintf(usrlib, sizeof(usrlib), LINUX_LIBC_PREFIX "%s/crti.o", multilib_prefix);
					dot = strrchr(usrlib, '.');
					assert(dot && dot > usrlib);

					dynarray_add(&state->ldflags_pre_user, ustrdup(usrlib));

					dot[-1] = 'n';
					dynarray_add(&state->ldflags_post_user, ustrdup(usrlib));
				}
			}

			if(vars->stdlibinc){
				dynarray_add(&state->args[mode_preproc], ustrdup("-isystem"));
				dynarray_add(&state->args[mode_preproc], ustrprintf("/usr/include/%s", multilib_prefix));
			}

			if(vars->ld_z.text){
				dynarray_add(&state->ldflags_pre_user, ustrdup("-z"));
				dynarray_add(&state->ldflags_pre_user, ustrdup("text"));
			}
			break;
		}

		case SYS_freebsd:
			if(vars->startfiles){
				dynarray_add(&state->ldflags_pre_user, ustrdup("/usr/lib/crt1.o"));
				dynarray_add(&state->ldflags_pre_user, ustrdup("/usr/lib/crti.o"));
				dynarray_add(&state->ldflags_pre_user, ustrdup("/usr/lib/crtbegin.o"));
				dynarray_add(&state->ldflags_post_user, ustrdup("/usr/lib/crtend.o"));
				dynarray_add(&state->ldflags_post_user, ustrdup("/usr/lib/crtn.o"));
			}
			if(vars->defaultlibs){
				dynarray_add(&state->ldflags_post_user, ustrdup("-lc"));
			}
			break;

		case SYS_darwin:
		{
			int syslibroot_alloc;
			const char *syslibroot = darwin_syslibroot(&syslibroot_alloc);
			if(!syslibroot)
				fprintf(stderr, "couldn't find syslibroot\n");

			dynarray_add(&state->args[mode_compile], ustrdup("-mpreferred-stack-boundary=4"));
			dynarray_add(&state->args[mode_compile], ustrdup("-malign-is-p2")); /* 2^4 = 16 byte aligned */
			dynarray_add(&state->args[mode_compile], ustrdup("-fforce-va_list-type"));
			dynarray_add(additional_argv, ustrdup("-fleading-underscore"));
			dynarray_add(additional_argv, ustrdup("-fpic"));

			/* no startfiles */
			if(vars->defaultlibs){
				dynarray_add(&state->ldflags_post_user, ustrdup("-lSystem"));

				if(vars->profile){
					dynarray_add(&state->ldflags_post_user, ustrdup("-lgcrt1.o"));
				}
			}
			if(vars->stdlibinc && syslibroot){
				dynarray_add(&state->args[mode_preproc], ustrdup("-isystem"));
				dynarray_add(&state->args[mode_preproc], ustrprintf("%s/usr/include", syslibroot));
			}

			if(syslibroot){
				dynarray_add(&state->ldflags_pre_user, ustrdup("-syslibroot"));
				dynarray_add(&state->ldflags_pre_user, ustrdup(syslibroot));
			}
			dynarray_add(&state->ldflags_pre_user, ustrdup("-macosx_version_min"));
			dynarray_add(
				&state->ldflags_pre_user,
				ustrdup(
					STRINGIFY(MACOS_VERSION_MAJ)
					"."
					STRINGIFY(MACOS_VERSION_MIN)
				)
			);

			if(vars->debug && vars->output){
				state->post_link = ustrprintf("dsymutil %s", vars->output);
			}

			if(!vars->shared){
				switch(vars->pie){
					case TRI_UNSET: /* default for 10.7 and later */
					case TRI_TRUE:
						dynarray_add(&state->ldflags_pre_user, ustrdup("-pie"));
						break;
					case TRI_FALSE:
						dynarray_add(&state->ldflags_pre_user, ustrdup("-no_pie"));
						break;
				}
			}

			if(vars->rdynamic)
				dynarray_add(&state->ldflags_pre_user, ustrdup("-export_dynamic"));

			paramshared = "-dylib";
			if(syslibroot_alloc)
				free((char *)syslibroot);
			break;
		}

		case SYS_cygwin:
			dynarray_add(additional_argv, ustrdup("-fleading-underscore"));
			break;
	}

	if(vars->shared)
		dynarray_add(&state->ldflags_pre_user, ustrdup(paramshared));
	if(vars->static_)
		dynarray_add(&state->ldflags_pre_user, ustrdup(paramstatic));

	/*
	switch(triple->arch){
		case ARCH_x86_64:
		case ARCH_i386:
			break;
		case ARCH_arm:
			ucc_initflags="-fshort-enums $ucc_initflags";
			break;
	}
	*/
}

static void usage(void)
{
	fprintf(stderr, "Staging\n");
	fprintf(stderr, "  -fsyntax-only: Only run preprocessor and compiler, with no output\n");
	fprintf(stderr, "  -E: Only run preprocessor\n");
	fprintf(stderr, "  -S: Only run preprocessor and compiler\n");
	fprintf(stderr, "  -c: Only run preprocessor, compiler and assembler\n");
	fprintf(stderr, "  -fuse-cpp=...: Specify a preprocessor executable to use\n");
	fprintf(stderr, "  -time: Output time for each stage\n");
	fprintf(stderr, "  -wrapper exe,arg1,...: Prefix stage commands with this executable and arguments\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Target options\n");
	fprintf(stderr, "  -target target: Compile as-if for the given target (specified as a partial target-triple)\n");
	fprintf(stderr, "  -dumpmachine: Display the current machine's detected target triple\n");
	fprintf(stderr, "  -m[no-]multilib: Assume a multilib installation\n");
	fprintf(stderr, "  -mmusl / -mglibc: Target the specified libc's dynamic linker\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Input options\n");
	fprintf(stderr, "  -xc: Treat input as C\n");
	fprintf(stderr, "  -xcpp-output: Treat input as preprocessor output\n");
	fprintf(stderr, "  -xasm, -xassembler: Treat input as assembly\n");
	fprintf(stderr, "  -xnone: Revert to inferring input based on file extension\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Output options\n");
	fprintf(stderr, "  -o file: Output file\n");
	fprintf(stderr, "  -shared: Output a shared library\n");
	fprintf(stderr, "  -static: Only link with static libraries (may be used with -shared)\n");
	fprintf(stderr, "  -pie/-no-pie: Tell the linker to emit a position independent executable (or not)\n");
	fprintf(stderr, "  -###: Output what would be done, do nothing\n");
	fprintf(stderr, "  -v: Output commands before invoking them\n");
	fprintf(stderr, "  -save-temps: Save temporary files for each stage\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Argument passing\n");
	fprintf(stderr, "  -Wp,... -Xpreprocessor ...: Pass to preprocessor\n");
	fprintf(stderr, "  -Wc,...:                    Pass to compiler\n");
	fprintf(stderr, "  -Wa,... -Xassembler ...:    Pass to assembler\n");
	fprintf(stderr, "  -Wl,... -Xlinker ...:       Pass to linker\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Disabling standard inputs\n");
	fprintf(stderr, "  -nostdlib: Don't link with the standard libraries or startup runtime\n");
	fprintf(stderr, "  -nostartfiles: Don't link with the startup runtime\n");
	fprintf(stderr, "  -nodefaultlibs: Don't link with the standard libraries\n");
	fprintf(stderr, "  -nostdinc: Don't include the builtin or standard header path\n");
	fprintf(stderr, "  -nostdlibinc: Don't include the standard header path\n");
	fprintf(stderr, "  -nobuiltininc: Don't include the builtin header path\n");
}

static int infer_target_from_argv0(struct triple *triple, const char *argv0)
{
	char *slash;
	char copy[64];
	int i;

	slash = strrchr(argv0, '/');
	if(slash)
		argv0 = slash + 1;

	xsnprintf(copy, sizeof copy, "%s", argv0);

	/* try with one trailing -, then with two (e.g. ucc-ar, etc), then give up */
	for(i = 0; i < 2; i++){
		const char *bad;
		char *p = strrchr(copy, '-');
		if(p)
			*p = '\0';

		if(triple_parse(copy, triple, &bad))
			return 1;
	}

	return 0;
}

static void add_library_path(struct ucc *const state)
{
	const char *library_path = getenv("LIBRARY_PATH");
	const char *p, *colon;

	if(!library_path)
		return;

	for(p = library_path; *p; p = colon + 1){
		char *dir;

		colon = strchr(p, ':');

		if(*p == ':')
			continue;

		dir = colon
			? ustrprintf("-L%.*s", (int)(colon - p), p)
			: ustrprintf("-L%s", p);

		dynarray_add(&state->args[mode_link], dir);

		if(!colon)
			break;
	}
}

static void add_pthread(struct ucc *const state)
{
	dynarray_add(&state->args[mode_preproc], ustrdup("-D_REENTRANT=1"));
	dynarray_add(&state->args[mode_link], ustrdup("-lpthread"));
}

int main(int argc, char **argv)
{
	size_t i;
	struct ucc state = { 0 };
	struct ucc argstate = { 0 };
	struct uccvars vars = { 0 };
	int *assumptions;
	int current_assumption;
	int output_given;
	struct triple triple;
	char **additional_argv = NULL;

	ucc_static_assert(tag1, countof(argstate.args) == mode_link + 1);
	(void)(check_tag1 *)0;

	argv0 = argv[0];
	if(argc <= 1){
usage:
		fprintf(stderr, "Usage: %s [options] input(s)\n", *argv);
		return 1;
	}

	argstate.mode = mode_link;
	current_assumption = -1;
	assumptions = umalloc((argc - 1) * sizeof(*assumptions));

	vars_default(&vars);

	orig_umask = umask(0077); /* prevent reading of the temporary files we create */

	/* we don't want the initial temporary fname "/tmp/tmp.xyz" tracked
	 * or showing up in error messages */
	dynarray_add(&state.args[mode_compile], ustrdup("-fno-track-initial-fname"));

	/* we must parse argv first for things like -nostdinc and -target.
	 * then we can initialise based on -target, then we need to
	 * append argv's inputs, etc onto the state from -target
	 *
	 * e.g.
	 *   cc -fsigned-char -target x86_64-linux ...
	 *
	 * -fsigned-char still takes effect, so target defaults don't override based
	 * on position on the command line
	 */
	parse_argv(argc - 1, argv + 1, &argstate, &vars, assumptions, &current_assumption);
	if(vars.help){
		fprintf(stderr, "dumping help:\n");
		fprintf(stderr, "--- cpp ---\n");
		preproc("--help", "/dev/null", NULL, 1);
		fprintf(stderr, "--- cc1 ---\n");
		compile("--help", "/dev/null", NULL, 1);
		fprintf(stderr, "--- ucc ---\n");
		usage();
		return 2;
	}

	if(vars.target){
		const char *bad;
		if(!triple_parse(vars.target, &triple, &bad)){
			fprintf(stderr, "couldn't parse target triple: %s\n", bad);
			return 1;
		}
	}else if(infer_target_from_argv0(&triple, argv[0])){
		/* done */
	}else{
		const char *unparsed;
		if(!triple_default(&triple, &unparsed)){
			fprintf(stderr, "couldn't get target triple: %s\n",
					unparsed ? unparsed : strerror(errno));
			return 1;
		}
	}
	if(vars.dumpmachine){
		printf("%s\n", triple_to_str(&triple, /* no vendor */0));
		return 0;
	}

	output_given = !!vars.output;
	if(!vars.output && argstate.mode == mode_link)
		vars.output = "a.out";

	state_from_triple(&state, &additional_argv, &vars, &triple);
	if(additional_argv){
		parse_argv(
				dynarray_count(additional_argv),
				additional_argv,
				&state,
				&vars,
				assumptions,
				&current_assumption);
	}

	merge_states(&state, &argstate);
	if(state.mode >= mode_link)
		add_library_path(&state);
	if(vars.pthread)
		add_pthread(&state);

	{
		const int ninputs = dynarray_count(state.inputs);

		if(output_given && ninputs > 1 && (state.mode == mode_compile || state.mode == mode_assemb))
			die("can't specify an output with '-%c' and multiple inputs", MODE_ARG_CH(state.mode));

		if(state.syntax_only){
			if(output_given || state.mode != mode_link)
				die("-%c specified in syntax-only mode",
						vars.output ? 'o' : MODE_ARG_CH(state.mode));

			state.mode = mode_compile;
			vars.output = "/dev/null";
		}

		if(ninputs == 0)
			goto usage;
	}


	if(vars.output && state.mode == mode_preproc && !strcmp(vars.output, "-"))
		vars.output = NULL;
	/* other case is -S, which is handled in rename_files */

	if(state.backend){
		/* -emit=... stops early */
		state.mode = mode_compile;
		if(!output_given)
			vars.output = "-";
	}

	if(state.isystems){
		const char **i;
		for(i = state.isystems; *i; i++){
			dynarray_add(&state.args[mode_preproc], ustrdup("-isystem"));
			dynarray_add(&state.args[mode_preproc], ustrdup(*i));
		}

		dynarray_free(const char **, state.isystems, NULL);
	}

	/* custom include paths */
	if(state.includes)
		dynarray_add_tmparray(&state.args[mode_preproc], state.includes);

	/* got arguments, a mode, and files to link */
	process_files(&state, assumptions, vars.output);

	for(i = 0; i < countof(state.args); i++)
		dynarray_free(char **, state.args[i], free);
	dynarray_free(char **, state.inputs, NULL);
	dynarray_free(char **, state.ldflags_pre_user, free);
	dynarray_free(char **, state.ldflags_post_user, free);
	free(assumptions);

	return 0;
}
