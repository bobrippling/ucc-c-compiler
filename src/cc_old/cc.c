#define _POSIX_C_SOURCE 200112L
#define _XOPEN_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdarg.h>
#include <ctype.h>

#include "cc.h"
#include "../util/alloc.h"
#include "../util/dynarray.h"

#define DEFAULT_OUTPUT "a.out"
#define LIB_PATH "/../../lib/"
#define CPP "cpp2"

#define TMP2(s, sz, pre, post) \
		snprintf(s, sz, pre "%d." post, getpid())

#define TMP(s, pre, post) TMP2(s, sizeof s, pre, post)

#define MODE_TO_STR(m) \
	(const char *[]){    \
		"preprocess",      \
		"compile",         \
		"assemble",        \
		"link" }[m]

#define LIBS        \
		"stdio",        \
		"stdlib",       \
		"string",       \
		"unistd",       \
		"syscall",      \
		"signal",       \
		"assert",       \
		"ctype",        \
		"dirent",       \
		"ucc",          \
		"alloca",       \
		"sys/fcntl",    \
		"sys/wait",     \
		"sys/mman",     \
		"sys/socket",

enum mode
{
	MODE_PREPROCESS,
	MODE_COMPILE,
	MODE_ASSEMBLE,
	MODE_LINK,
	MODE_UNKNOWN
};

struct
{
	char arg;
	char suffix;
} modes[] = {
	[MODE_PREPROCESS] = { 'E', '\0' }, /* stdout */
	[MODE_COMPILE]    = { 'S',  's' },
	[MODE_ASSEMBLE]   = { 'c',  'o' },
	[MODE_LINK]       = {  0,    0  }
};

int no_startfiles = 0, no_stdlib = 0;
int no_warn = 0;
int verbose = 0;
int debug   = 0;
int do_rm   = 1;
char *backend  = "";
char *frontend = "";

char *precmd = "";

enum mode out_mode;
enum mode start_mode;
char *argv0;

char where[1024];
char  f_e[32]; /* "/tmp/ucc_$$.s"; */
char  f_s[32]; /* "/tmp/ucc_$$.s"; */
char  f_o[32]; /* "/tmp/ucc_$$.o"; */
const char *f;
char *stdlib_files;

char *args[4] = { "", "", "", "" };

void (*ice)() = exit;

void die(const char *fmt, ...)
{
	va_list l;
	if(fmt){
		va_start(l, fmt);
		vfprintf(stderr, fmt, l);
		va_end(l);
	}
	exit(1);
}

void args_add(enum mode m, const char *arg)
{
	if(!*args[m]){
		args[m] = ustrdup(arg);
	}else{
		args[m] = urealloc(args[m], strlen(args[m]) + strlen(arg) + 2);

		strcat(args[m], " ");
		strcat(args[m], arg);
	}
}

void run(const char *cmd)
{
	int ret;

	if(verbose)
		fprintf(stderr, "run(\"%s\")\n", cmd);

	ret = system(cmd);

	if(ret){
		char *arg0 = ustrdup(cmd);
		char *p;

		for(p = arg0; isspace(*p); p++);

		*strchr(p, ' ') = '\0';

		if((p = strrchr(arg0, '/')))
			p++;
		else
			p = arg0;


		if(WIFSIGNALED(ret))
			die("%s caught signal %d\n", p, WTERMSIG(ret));

		die(verbose ? "%s returned %d\n" : NULL, p, ret);
	}
}

void unlink_files()
{
#define RM(m, path) if(out_mode == m) return; unlink(path)
	RM(MODE_PREPROCESS, f_e);
	RM(MODE_COMPILE,    f_s);
	RM(MODE_ASSEMBLE,   f_o);
	RM(MODE_LINK,       f);
}

char *gen_stdlib_files(void)
{
	const char *base = LIB_PATH;
	const char *names[] = {
		LIBS
		NULL
	};
	const int blen = strlen(where) + strlen(base);
	char *ret;
	int len;
	int i;

	for(i = len = 0; names[i]; i++)
		len += blen + strlen(names[i]) + 3; /* for ".o " */

	ret = umalloc(len + 1);

	for(i = len = 0; names[i]; i++)
		len += sprintf(ret + len, "%s%s%s.o ", where, base, names[i]);

	return ret;
}

int gen(const char *input, const char *output)
{
	char cmd[4096];

	TMP(f_e, "/tmp/ucc_", "i");
	TMP(f_s, "/tmp/ucc_", "s");
	TMP(f_o, "/tmp/ucc_", "o");
	f = output ? output : DEFAULT_OUTPUT;

	if(do_rm)
		atexit(unlink_files);

#define RUN(local, fmt, ...) \
		snprintf(cmd, sizeof cmd, "%s %s" fmt, precmd, local ? where : "", __VA_ARGS__); \
		run(cmd)

#define SHORTEN_OUTPUT(m, path) \
	if(out_mode == m) \
		snprintf(path, sizeof path, "%s", output)

#define START_MODE(m, lbl, file) \
	case m: \
		RUN(0, "cp %s %s", input, file); \
		goto lbl

	switch(start_mode){
		START_MODE(MODE_ASSEMBLE, start_assemble, f_s);
		START_MODE(MODE_COMPILE,  start_compile,  f_e);
		START_MODE(MODE_LINK,     start_link,     f_o);

		case MODE_PREPROCESS:
		case MODE_UNKNOWN:
				break;
	}

	SHORTEN_OUTPUT(MODE_PREPROCESS, f_e);
	RUN(1, CPP "/cpp %s -I'%s" LIB_PATH "' %s -o %s %s", verbose ? "-d" : "", where, args[MODE_PREPROCESS], f_e, input);
	if(out_mode == MODE_PREPROCESS)
		return 0;

start_compile:
	SHORTEN_OUTPUT(MODE_COMPILE, f_s);
	RUN(1, "cc1/cc1 %s %s %s %s -o %s %s", no_warn ? "-w" : "", *backend ? "-X" : "", backend, args[MODE_COMPILE], f_s, f_e);
	if(out_mode == MODE_COMPILE)
		return 0;

#define DEBUG_STR debug ? "-g" : ""

start_assemble:
	SHORTEN_OUTPUT(MODE_ASSEMBLE, f_o);
	RUN(0, UCC_NASM " -f " UCC_ARCH " %s %s -o %s %s", args[MODE_ASSEMBLE], DEBUG_STR, f_o, f_s);
	if(out_mode == MODE_ASSEMBLE)
		return 0;

start_link:
	RUN(0, UCC_LD " " UCC_LDFLAGS " %s -o %s %s %s %s%s %s", DEBUG_STR, f, f_o,
			no_stdlib     ? "" : stdlib_files,
			no_startfiles ? "" : where,
			no_startfiles ? "" : "/../lib/crt.o",
			args[MODE_LINK]
			);
	return 0;
}

void usage(char *prog)
{
	fprintf(stderr,
			"Usage: %s [-Wwarning...] [-foption...] [-[ESc]] [-o output] input\n"
			"Other options:\n"
			"  -nost{dlib,artfiles} - don't like with stdlib/crt.o\n"
			"  -no-rm - don't remove temporary files\n"
			"  -d - run in debug/verbose mode\n"
			"  -X backend - specify cc1 backend\n"
			"  -x frontend - specify starting point (c, cpp-output and asm)\n",
			prog);

	exit(1);
}

void wrap_linker(int argc, char **argv)
{
	/*
	 * steal -o a.out argument
	 * forward everything else to *argv
	 */
	int i, nfiles;
	char *output;
	char **args, **files;
	char **tmps;
	char *args_str;

	args = files = tmps = NULL;

	for(i = 1; i < argc; i++){
		if(!strncmp(argv[i], "-o", 2)){
			output = argv[i][2] ? argv[i] + 2 : argv[++i];
			if(!output)
				usage(*argv);
		}else{
			dynarray_add(*argv[i] == '-' ? (void ***)&args : (void ***)&files, argv[i]);
		}
	}

	if(!output)
		output = DEFAULT_OUTPUT;

	nfiles = dynarray_count((void **)files);

	if(args){
		int len = 1;
		char *p;

		for(i = 0; args[i]; i++)
			len += strlen(args[i]) + 1;

		p = args_str = umalloc(len);

		for(i = 0; args[i]; i++)
			p += sprintf(p, "%s ", args[i]);
	}else{
		args_str = ustrdup("");
	}

	tmps = umalloc((nfiles + 1) * sizeof *tmps);

	for(i = 0; i < nfiles; i++){
		char buf[256];
		int r;

		r = strlen(files[i]);

		if(r >= 3 && !strcmp(files[i] + r - 2, ".o")){
			tmps[i] = ustrdup(files[i]);
			continue; /* already in the right form */
		}

		tmps[i] = umalloc(32);
		TMP2(tmps[i], 32, "/tmp/ucc_", "f");

		snprintf(buf, sizeof buf, "%s -c -o %s %s %s",
				*argv,
				tmps[i],
				args_str, files[i]);

		run(buf);
	}

	free(args_str);
	dynarray_free((void ***)&args, NULL);
	dynarray_free((void ***)&files, NULL);

	/* link */
	{
		const char *pre = "ld -e _start -o";
		int len;
		char *link_cmd, *p;
		char *stdlib = gen_stdlib_files();

		len = 1 + strlen(pre) + 1 + strlen(output) + 1 + strlen(stdlib);

		for(i = 0; tmps[i]; i++)
			len += strlen(tmps[i]) + 1;

		link_cmd = umalloc(len);

		p = link_cmd + sprintf(link_cmd, "%s %s %s", pre, output, stdlib);

		free(stdlib);

		for(i = 0; tmps[i]; i++)
			p += sprintf(p, " %s", tmps[i]);

		run(link_cmd);
	}

	dynarray_free((void ***)&tmps, free);
}

int main(int argc, char **argv)
{
#define USAGE() usage(*argv)
	int assumed_start_mode = 0;
	char *input;
	char *output;
	char *p;
	int i;

	struct
	{
		char optn;
		char **ptr;
	} opts[] = {
		{ 'o', &output   },
		{ 'X', &backend  },
		{ 'x', &frontend }
	};

	argv0 = umalloc(strlen(*argv) + 1);
	strcpy(argv0, *argv);

	out_mode = MODE_LINK;
	input = output = NULL;

	for(i = 1; i < argc; i++)
		if(!strcmp(argv[i], "-d")){
			verbose = 1;
		}else if(!strcmp(argv[i], "-g")){
			debug = 1;
		}else if(!strcmp(argv[i], "-w")){
			no_warn = 1;
		}else if(!strncmp(argv[i], "-nost", 5)){
			if(!strcmp(argv[i] + 5, "artfiles"))
				no_startfiles = 1;
			else if(!strcmp(argv[i] + 5, "dlib"))
				no_stdlib = 1;
			else
				USAGE();
		}else if(!strcmp(argv[i], "--help")){
			USAGE();
		}else if(!strcmp(argv[i], "-no-rm")){
			do_rm = 0;
		}else if(!strncmp(argv[i], "--precmd=", 9)){
			precmd = argv[i] + 9;
		}else{
			if(argv[i][0] == '-'){
				unsigned int j;
				int found;
				char *arg = argv[i];

				switch(arg[1]){
					case 'W':
					{
						/* check for W%c, */
						char c;
						if(sscanf(argv[i], "-W%c,", &c) == 1){
							switch(c){
#define MAP(c, l) case c: arg += 4; goto arg_ ## l;
								MAP('p', preproc);
								MAP('a', assemb);
								MAP('l', linker);
#undef MAP
								default:
									fprintf(stderr, "argument \"%s\" assumed to be for cc1\n", argv[i]);
							}
						}
						/* else default to -Wsomething - add to cc1 */
					}

					case 'f':
						args_add(MODE_COMPILE, arg);
						continue;
					case 'D':
					case 'U':
					case 'I':
arg_preproc:
						args_add(MODE_PREPROCESS, arg);
						continue;

						/* no cases for as args yet */
arg_assemb:
						args_add(MODE_ASSEMBLE, arg);
						continue;

					case 'l':
					case 'L':
arg_linker:
						args_add(MODE_LINK, arg);
						continue;
				}

				if(!argv[i][2]){
					found = 0;
					for(j = 0; j < 3; j++)
						if(argv[i][1] == modes[j].arg){
							out_mode = j;
							found = 1;
							break;
						}

					if(found)
						continue;
				}

				found = 0;
				for(j = 0; j < sizeof(opts) / sizeof(opts[0]); j++)
					if(argv[i][1] == opts[j].optn){
						if(argv[i][2]){
							*opts[j].ptr = argv[i] + 2;
						}else{
							if(!argv[++i])
								USAGE();
							*opts[j].ptr = argv[i];
						}
						found = 1;
						break;
					}

				if(found)
					continue;
			}

			if(!input){
				input = argv[i];
			}else{
				wrap_linker(argc, argv);
				return 0;
			}
		}


	if(!input)
		USAGE();

	if(*frontend){
		if(!strcmp(frontend, "c"))
			start_mode = MODE_PREPROCESS;
		else if(!strcmp(frontend, "cpp-output"))
			start_mode = MODE_COMPILE;
		else if(!strcmp(frontend, "asm"))
			start_mode = MODE_ASSEMBLE;
		else
			USAGE();

		/* "asm-with-cpp", MODE??? */

	}else{
		/* figure out the start mode from the file name */
		assumed_start_mode = 1;

		start_mode = MODE_PREPROCESS;

		i = strlen(input);
		if(i >= 3){
			if(input[i - 2] == '.'){
				switch(input[i - 1]){

#define CHAR_MAP(c, m) \
				case c: start_mode = m; break
					CHAR_MAP('c', MODE_PREPROCESS);
					CHAR_MAP('i', MODE_COMPILE);
					CHAR_MAP('s', MODE_ASSEMBLE);
					CHAR_MAP('o', MODE_LINK);
#undef CHAR_MAP

					default:
						goto unknown_file;
				}
			}else{
	unknown_file:
				if(strcmp(input, "-"))
					fprintf(stderr, "%s: assuming input \"%s\" is c-source\n", argv0, input);
			}
		}else{
			goto unknown_file;
		}
	}

	/* "cc -Xprint" will act as in "cc -S -o- -Xprint" was given */
	if(!strcmp(backend, "print") && out_mode > MODE_COMPILE){
		out_mode = MODE_COMPILE;
		if(!output)
			output = "-";
	}

	if(out_mode < start_mode){
		if(assumed_start_mode){
			if(start_mode == MODE_LINK && out_mode == MODE_ASSEMBLE){
				fprintf(stderr, "%s: linker input files \"%s\" unused because linking not done\n",
						*argv, input);
				return 0;
			}else{
				fprintf(stderr, "%s: warning: output mode \"%s\" is past start mode \"%s\" - fixing\n",
						*argv, MODE_TO_STR(out_mode), MODE_TO_STR(start_mode));
			}
		}

		start_mode = out_mode;
	}

	if(!output){
		if(out_mode == MODE_PREPROCESS){
			output = "-";
		}else if(out_mode == MODE_LINK){
			output = DEFAULT_OUTPUT;
		}else{
			int len = strlen(input);

			output = umalloc(len + 3);

			if(input[len - 2] == '.'){
				strcpy(output, input);
				output[len - 1] = modes[out_mode].suffix;
			}else{
				sprintf(output, "%s.%c", input, modes[out_mode].suffix);
			}
		}
	}else if(!strcmp(output, "-") && out_mode >= MODE_ASSEMBLE){
		fprintf(stderr, "%s: warning: outputting to the file \"-\", not stdout\n", *argv);
	}

#if 0
	printf("out_mode = %s, input = %s, output = %s\n",
			out_mode == MODE_PREPROCESS ? "preprocess" :
			out_mode == MODE_COMPILE    ? "compile" :
			out_mode == MODE_ASSEMBLE   ? "assemble" :
			out_mode == MODE_LINK       ? "link" : "unknown",
			input, output);
	return 0;
#endif

	/* XXX: this will want stealing */
	if(readlink(argv[0], where, sizeof where) == -1)
		snprintf(where, sizeof where, "%s", argv[0]);
	/* dirname */
	p = strrchr(where, '/');
	if(p)
		*++p = '\0';

	stdlib_files = gen_stdlib_files();

	return gen(input, output);
}
