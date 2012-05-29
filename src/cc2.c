enum mode
{
	mode_preproc,
	mode_compile,
	mode_assemb,
	mode_link
};

struct cc_file
{
	char *in, *preproc, *compile, *assemb;
};

void create_file(struct cc_file *file, char *in)
{
	files[i].in = inputs[i];
}

int process_files(enum mode mode, char **inputs, char *output, char **args[4], char *backend)
{
	const int ninputs = dynarray_count((void **)inputs);
	int i;
	struct cc_file *files;

	files = umalloc(ninputs * sizeof *files);

	for(i = 0; i < ninputs; i++){
		create_file(&files[i], inputs[i]);

	}
}

int main(int argc, char **argv)
{
#define ADD_ARG(to) dynarray_add((void ***)&args[to], argv[i])

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

	for(i = 1; i < argc; i++){
		if(!strcmp(argv[i], "--")){
			while(++i < argc)
				dynarray_add((void ***)&inputs, argv[i]);
			break;

		}else if(*argv[i] == '-'){
			int found = 0;

			switch(argv[i][1]){
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

				case 'w':
				case 'f':
arg_cc1:
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

				case 'E': mode = mode_preproc;  continue;
				case 'S': mode = mode_compile;  continue;
				case 'c': mode = mode_assemble; continue;

				default:
					/* TODO: -nostdlib, -nostartfiles */
					die("TODO");
			}

			if(!argv[i][2]){
				for(j = 0; j < 3; j++)
					if(argv[i][1] == modes[j].arg){
						out_mode = j;
						found = 1;
						break;
					}

				if(found)
					continue;
			}

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

			if(!found)
				die("unrecognised option \"%s\"", argv[i]);
		}else{
			dynarray_add((void ***)&inputs, argv[i]);
		}
	}


	/* got arguments, a mode, and files to link */
	return process_files(mode, inputs, output, arg, backend);
}
