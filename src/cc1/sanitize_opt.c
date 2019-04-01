#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../util/alloc.h"

#include "sanitize_opt.h"

#include "fopt.h"
#include "cc1.h"

enum san_opts cc1_sanitize = 0;
char *cc1_sanitize_handler_fn;

void sanitize_opt_add(const char *argv0, const char *san)
{
	if(!strcmp(san, "undefined")){
		cc1_sanitize |= SAN_UBSAN;
		cc1_fopt.trapv = 1;
	}else{
		fprintf(stderr, "%s: unknown sanitize option '%s'\n", argv0, san);
		exit(1);
	}
}

void sanitize_opt_set_error(const char *argv0, const char *handler)
{
	free(cc1_sanitize_handler_fn);
	cc1_sanitize_handler_fn = NULL;

	if(!strcmp(handler, "trap")){
		/* fine */
	}else if(!strncmp(handler, "call=", 5)){
		cc1_sanitize_handler_fn = ustrdup(handler + 5);

		if(!*cc1_sanitize_handler_fn){
			fprintf(stderr, "%s: empty sanitize function handler\n", argv0);
			exit(1);
		}

	}else{
		fprintf(stderr, "%s: unknown sanitize handler '%s'\n", argv0, handler);
		exit(1);
	}
}
