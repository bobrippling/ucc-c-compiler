enum mode
{
	MODE_PREPROCESS,
	MODE_COMPILE,
	MODE_ASSEMBLE,
	MODE_LINK,
};

void get_frontend(char *fend, enum mode *pmode)
{
	/* figure out the start mode from the file name */
	enum mode mode = MODE_PREPROCESS;

	i = strlen(inputs[0]);
	if(i >= 3){
		if(input[i - 2] == '.'){
			switch(input[i - 1]){

#define CHAR_MAP(c, m) \
			case c: start_mode = m; break
				CHAR_MAP('c', MODE_PREPROCESS);
				CHAR_MAP('e', MODE_COMPILE);
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

int main(int argc, char **argv)
{
	char **inputs;
	char *frontend, *backend;
	enum mode start_mode, end_mode, original_end;
	int i;

	args_process(argc, argv, &inputs, &frontend, &backend);

	if(!*inputs)
		goto usage;


	get_frontend(frontend, &start_mode);
	original_end = end_mode;

	/* "cc -Xprint" will act as in "cc -S -o- -Xprint" was given */
	if(!strcmp(backend, "print") && out_mode > MODE_COMPILE){
		out_mode = MODE_COMPILE;
		if(!output)
			output = "-";
	}

	if(inputs[1]){
		/* defer linking */
		end_mode = MODE_ASSEMBLE;
	}

	for(i = 0; inputs[i]; i++)
		gen_file(inputs[i], end_mode);

	if(inputs[1]){
		fprintf(stderr, "TODO: link all\n");
	}


	return 0;
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
