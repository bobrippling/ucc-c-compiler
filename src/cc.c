#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

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


int main(int argc, char **argv)
{
	enum mode mode;
	const char *input;
	char *output;
	FILE *finput, *foutput;
	int i;

	mode  = MODE_LINK;
	input = output = NULL;

	for(i = 1; i < argc; i++)
		if(strlen(argv[i]) == 2 && *argv[i] == '-'){
			if(argv[i][1] == 'o'){
				if(argv[++i]){
					output = argv[i];
				}else{
					goto usage;
				}
			}else{
				int j;
				mode = MODE_UNKNOWN;
				for(j = 0; j < 3; j++)
					if(argv[i][1] == modes[j].arg){
						mode = j;
						break;
					}
				if(mode == MODE_UNKNOWN)
					goto usage;
			}
		}else if(!input){
			input = argv[i];
		}else{
		usage:
			fprintf(stderr, "Usage: %s [-c] [-o output] input\n", *argv);
			return 1;
		}

	if(!input)
		goto usage;

	finput = fopen(input, "r");
	if(!finput){
		if(!strcmp(input, "-")){
			finput = stdin;
		}else{
			fprintf(stderr, "open %s: %s\n", input, strerror(errno));
			return 1;
		}
	}

	if(!output){
		if(mode == MODE_PREPROCESS){
			foutput = stdout;
		}else if(mode == MODE_LINK){
			foutput = fopen("a.out", "w");
			if(!foutput){
				fprintf(stderr, "open %s: %s\n", "a.out", strerror(errno));
				return 1;
			}
		}else{
			int len   = strlen(input);

			output = malloc(len + 3);

			if(input[len - 2] == '.'){
				strcpy(output, input);
				output[len - 1] = modes[mode].suffix;
			}else{
				sprintf(output, "%s.%c", input, modes[mode].suffix);
			}

			foutput = fopen(output, "w");
			if(!foutput){
				fprintf(stderr, "open %s: %s\n", output, strerror(errno));
				return 1;
			}
		}
	}

	printf("mode = %s, input = %s, output = %s\n",
			mode == MODE_PREPROCESS ? "preprocess" :
			mode == MODE_COMPILE    ? "compile" :
			mode == MODE_ASSEMBLE   ? "assemble" :
			mode == MODE_LINK       ? "link" : "unknown",
			input, output);

	return 0;
}
