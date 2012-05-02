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

#include "cc.h"
#include "util/alloc.h"

#define LIB_PATH "/../lib/"
#define CPP "cpp2"

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

		*strchr(arg0, ' ') = '\0';
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

#define TMP(s, pre, post) \
		snprintf(s, sizeof s, pre "%d." post, getpid())

	TMP(f_e, "/tmp/ucc_", "i");
	TMP(f_s, "/tmp/ucc_", "s");
	TMP(f_o, "/tmp/ucc_", "o");
	f = output ? output : "a.out";

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
	RUN(0, UCC_AS " %s %s -o %s %s", args[MODE_ASSEMBLE], DEBUG_STR, f_o, f_s);
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


int main(int argc, char **argv)
{
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
				goto usage;
		}else if(!strcmp(argv[i], "--help")){
			goto usage;
		}else if(!strcmp(argv[i], "-no-rm")){
			do_rm = 0;
		}else if(!strncmp(argv[i], "--precmd=", 9)){
			precmd = argv[i] + 9;
		}else{
			if(argv[i][0] == '-'){
				unsigned int j;
				int found;

				switch(argv[i][1]){
					case 'f':
					case 'W':
						args_add(MODE_COMPILE, argv[i]);
						continue;
					case 'D':
					case 'U':
					case 'I':
						args_add(MODE_PREPROCESS, argv[i]);
						continue;
					case 'l':
					case 'L':
						args_add(MODE_LINK, argv[i]);
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
								goto usage;
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
			usage:
				fprintf(stderr,
						"Usage: %s [-Wwarning...] [-foption...] [-[ESc]] [-o output] input\n"
						"Other options:\n"
						"  -nost{dlib,artfiles} - don't like with stdlib/crt.o\n"
						"  -no-rm - don't remove temporary files\n"
						"  -d - run in debug/verbose mode\n"
						"  -X backend - specify cc1 backend\n"
						"  -x frontend - specify starting point (c, cpp-output and asm)\n",
						*argv);
				return 1;
			}
		}


	if(!input)
		goto usage;

	if(*frontend){
		if(!strcmp(frontend, "c"))
			start_mode = MODE_PREPROCESS;
		else if(!strcmp(frontend, "cpp-output"))
			start_mode = MODE_COMPILE;
		else if(!strcmp(frontend, "asm"))
			start_mode = MODE_ASSEMBLE;
		else
			goto usage;

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
			fprintf(stderr, "%s: warning: output mode \"%s\" is past start mode \"%s\" - fixing\n",
					*argv, MODE_TO_STR(out_mode), MODE_TO_STR(start_mode));
		}

		start_mode = out_mode;
	}

	if(!output){
		if(out_mode == MODE_PREPROCESS){
			output = "-";
		}else if(out_mode == MODE_LINK){
			output = "a.out";
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

	if(readlink(argv[0], where, sizeof where) == -1)
		snprintf(where, sizeof where, "%s", argv[0]);
	/* dirname */
	p = strrchr(where, '/');
	if(p)
		*++p = '\0';


	stdlib_files = gen_stdlib_files();

	return gen(input, output);
}
