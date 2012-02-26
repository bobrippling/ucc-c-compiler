void args_process(int argc, char **argv)
{
	int i;

	for(i = 1; i < argc; i++){
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
		}else if(!strcmp(argv[i], "-no-rm")){
			do_rm = 0;
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

			inputs = argv + i;
			break;
		}
	}
}
