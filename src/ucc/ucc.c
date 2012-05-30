#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include "ucc.h"
#include "ucc_ext.h"
#include "ucc_lib.h"
#include "../util/alloc.h"
#include "../util/dynarray.h"

enum mode
{
	mode_preproc,
	mode_compile,
	mode_assemb,
	mode_link
};

struct cc_file
{
	char *in;

	char *preproc,
			 *compile,
			 *assemb;

	char *out;
};

const char *argv0;


char *tmpfilenam()
{
	char *r = tmpnam(NULL);

	if(!r)
		die("tmpfile():");

	return ustrdup(r);
}

void create_file(struct cc_file *file, char *in)
{
	char *ext;

	file->in = in;

	ext = strrchr(in, '.');
	if(ext && ext[1] && !ext[2]){
		switch(ext[1]){
			case 'c':
				file->preproc = tmpfilenam();
			case 'i':
				file->compile = tmpfilenam();
			case 's':
				file->assemb = tmpfilenam();
				file->out = file->assemb;
				break;

			case 'o':
			default:
assume_obj:
				/* else assume it's already an object file */
				file->out = file->in;
		}
	}else{
		goto assume_obj;
	}
}

void free_file(struct cc_file *file)
{
	/*free(file->in);*/
	free(file->preproc);
	free(file->compile);
	free(file->assemb);
	/*free(file->out);*/
}

void gen_obj_file(struct cc_file *file, char **args[4])
{
	if(file->assemb){
		char *in = file->in;

		if(file->compile){
			if(file->preproc){
				preproc(in, file->preproc, args[mode_preproc]);
				in = file->preproc;
			}
			compile(in, file->compile, args[mode_compile]);
			in = file->compile;
		}
		assemble(in, file->assemb, args[mode_assemb]);
	}
}

void process_files(enum mode mode, char **inputs, char *output, char **args[4], char *backend)
{
	const int ninputs = dynarray_count((void **)inputs);
	int i;
	struct cc_file *files;
	char **links;

	if(mode != mode_link)
		die("TODO: mode");

	if(backend)
		die("TODO: backend");

	files = umalloc(ninputs * sizeof *files);

	links = objfiles_stdlib();
	dynarray_add((void ***)&links, objfiles_start());


	for(i = 0; i < ninputs; i++){
		create_file(&files[i], inputs[i]);

		gen_obj_file(&files[i], args);

		dynarray_add((void ***)&links, files[i].out);
	}

	link(links, output, args[mode_link]);

	dynarray_free((void ***)&links, NULL);
	for(i = 0; i < ninputs; i++)
		free_file(&files[i]);
	free(files);
}

int main(int argc, char **argv)
{
	enum mode mode = mode_link;
	int i;
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

	argv0 = argv[0];

	if(argc <= 1){
		fprintf(stderr, "Usage: %s [options] input(s)\n", *argv);
		return 1;
	}

	for(i = 1; i < argc; i++){
		if(!strcmp(argv[i], "--")){
			while(++i < argc)
				dynarray_add((void ***)&inputs, argv[i]);
			break;

		}else if(*argv[i] == '-'){
			int found = 0;
			unsigned int j;
			char *arg = argv[i];

			switch(arg[1]){
				case 'W':
				{
					/* check for W%c, */
					char c;
					if(sscanf(arg, "-W%c,", &c) == 1){
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
				}

#define ADD_ARG(to) dynarray_add((void ***)&args[to], arg)

				case 'w':
				case 'f':
/*arg_cc1:*/
					ADD_ARG(mode_compile);
					continue;

				case 'D':
				case 'U':
				case 'I':
arg_cpp:
					ADD_ARG(mode_preproc);
					continue;

arg_asm:
					ADD_ARG(mode_assemb);
					continue;

				case 'l':
				case 'L':
arg_ld:
					ADD_ARG(mode_link);
					continue;

				case 'E': mode = mode_preproc; continue;
				case 'S': mode = mode_compile; continue;
				case 'c': mode = mode_assemb;  continue;

				default:
					/* TODO: -nostdlib, -nostartfiles */
					die("TODO: %s", arg);
			}

			for(j = 0; j < sizeof(opts) / sizeof(opts[0]); j++)
				if(argv[i][1] == opts[j].optn){
					if(argv[i][2]){
						*opts[j].ptr = argv[i] + 2;
					}else{
						if(!argv[++i])
							die("need argument for %s", argv[i - 1]);
						*opts[j].ptr = argv[i];
					}
					found = 1;
					break;
				}

			if(!found)
				die("unrecognised option \"%s\"", argv[i]);
		}else{
			dynarray_add((void ***)&inputs, argv[i]);
		}
	}

	/* got arguments, a mode, and files to link */
	process_files(mode, inputs, output, args, backend);

	return 0;
}

void die(const char *fmt, ...)
{
	const int len = strlen(fmt);
	const int err = len > 0 && fmt[len - 1] == ':' ;
	va_list l;

	va_start(l, fmt);
	vfprintf(stderr, fmt, l);
	va_end(l);

	if(err)
		fprintf(stderr, ": %s", strerror(errno));

	fputc('\n', stderr);

	exit(1);
}

void ice(const char *fmt)
{
	die("%s", fmt);
}
