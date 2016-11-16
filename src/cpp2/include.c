#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "../util/dynarray.h"
#include "../util/alloc.h"
#include "../util/util.h"

#include "include.h"

/* CPP_DIE() */
#include "main.h"
#include "preproc.h"

static char **include_dirs;
static char **include_dirs_sys;

void include_add_dir(char *d, int sysh)
{
	if(sysh){
		dynarray_add(&include_dirs_sys, d);
	}else{
		dynarray_add(&include_dirs, d);
	}
}

static FILE *wrapped_fopen(const char *fnam)
{
	FILE *f;
	char *rslash = strrchr(fnam, '/');

	if((rslash ? rslash[1] : *fnam) == '\0')
		CPP_DIE("empty filename");

	f = fopen(fnam, "r");
	if(f)
		return f;

	if(errno != ENOENT)
		CPP_DIE("open: %s:", fnam);

	return NULL;
}

static FILE *include_search(
		const char *fname,
		char **dirs,
		char **const final_path)
{
	size_t i;

	for(i = 0; dirs && dirs[i]; i++){
		char *path = ustrprintf("%s/%s", dirs[i], fname);
		FILE *f = wrapped_fopen(path);

		trace("  trying %s...\n", path);

		if(f){
			trace("  found @ %s\n", path);
			*final_path = path;
			return f;
		}
		free(path);
	}

	return NULL;
}

FILE *include_fopen(
		const char *curdir,
		const char *fname,
		int is_angle,
		char **final_path,
		int *is_sysh)
{
	/* "" -> curdir, includes, isystems
	 * <> ->         includes, isystems */
	FILE *f;

	*final_path = NULL;

	trace("include %c%s%c\n",
			(is_angle ? '<' : '"'),
			fname,
			(is_angle ? '>' : '"'));

	if(*fname == '/'){
		*final_path = ustrdup(fname);
		*is_sysh = 0;
		trace("  absolute path - using\n");
		return wrapped_fopen(fname);
	}

	if(!is_angle){
		*final_path = ustrprintf("%s/%s", curdir, fname);

		f = wrapped_fopen(*final_path);
		if(f){
			trace("  found (local) @ %s\n", *final_path);
			*is_sysh = 0;
			return f;
		}

		free(*final_path);
		*final_path = NULL;
	}

	trace(" trying -I dirs:\n");
	f = include_search(fname, include_dirs, final_path);
	if(f){
		/* this disagrees with clang/gcc - for them, even if found with -I...,
		 * if the directory in which `f` is in, is also in -isystem, then it's a
		 * system header */
		*is_sysh = 0;
		return f;
	}

	trace(" trying -isystem dirs:\n");
	*is_sysh = 1; /* "" or <>, either way it's in -isystem, so is a sysh */
	return include_search(fname, include_dirs_sys, final_path);
}
