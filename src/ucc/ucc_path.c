#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include <unistd.h>

#include "ucc.h"
#include "ucc_path.h"
#include "../util/alloc.h"

static void
bname(char *path)
{
	char *p = strrchr(path, '/');
	if(p)
		p[1] = '\0';
}

static char *
dirname_ucc(void)
{
	static char where[1024];

	if(!where[0]){
		char link[1024];
		ssize_t nb;

		if((nb = readlink(argv0, link, sizeof link)) == -1){
			snprintf(where, sizeof where, "%s", argv0);
		}else{
			char *argv_dup;

			link[nb] = '\0';
			/* need to tag argv0's dirname onto the start */
			argv_dup = ustrdup(argv0);

			bname(argv_dup);

			snprintf(where, sizeof where, "%s/%s", argv_dup, link);

			free(argv_dup);
		}

		/* dirname */
		bname(where);
	}

	return where;
}

static char *path_prepend(const char *pre, const char *path)
{
	/* ../{cc1,cpp2}/... */
	char *buf = umalloc(strlen(pre) + strlen(path) + 2);

	sprintf(buf, "%s/%s", pre, path);

	return buf;
}

static char *path_prepend_relative_nofree(const char *path)
{
	return path_prepend(dirname_ucc(), path);
}

char *path_prepend_relative(char *path)
{
	char *r = path_prepend_relative_nofree(path);
	free(path);
	return r;
}

char *cmdpath_resolve(const struct cmdpath *p, cmdpath_exec_fn **fn)
{
	switch(p->type){
		case USE_PATH:
			if(fn) *fn = execvp;
			return ustrdup(p->path);

		case RELATIVE_TO_UCC:
			if(fn) *fn = execv;
			return path_prepend_relative_nofree(p->path);

		case FROM_Bprefix:
			if(fn) *fn = execv;
			return path_prepend(Bprefix, p->path);
	}

	return NULL;
}

void cmdpath_initrelative(
		struct cmdpath *p,
		const char *bprefix,
		const char *ucc_relative)
{
	if(Bprefix){
		p->path = bprefix;
		p->type = FROM_Bprefix;
	}else{
		p->path = ucc_relative;
		p->type = RELATIVE_TO_UCC;
	}
}

const char *cmdpath_type(enum cmdpath_type t)
{
	switch(t){
		case RELATIVE_TO_UCC: return "ucc-relative";
		case USE_PATH: return "$PATH-search";
		case FROM_Bprefix: return "Bprefix-relative";
	}
	return NULL;
}
