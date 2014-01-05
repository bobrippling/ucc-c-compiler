#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>

#include <sys/types.h>
#include <dirent.h>

#include <unistd.h>

#include "ucc.h"
#include "ucc_path.h"
#include "../util/alloc.h"

static int path_search_self(char buf[], size_t len)
{
	char *path = getenv("PATH");
	char *p;

	*buf = '\0';

	if(!path)
		die("no $PATH to find ucc");
	path = ustrdup(path);

	for(p = strtok(path, ":"); p; p = strtok(NULL, ":")){
		DIR *d = opendir(p);
		struct dirent *ent;

		if(!d)
			continue;

		while(errno = 0, ent = readdir(d)){
			if(!strcmp(ent->d_name, argv0)){
				int written = snprintf(buf, len, "%s/%s", p, ent->d_name);

				if((size_t)written >= len)
					die("ucc path too long: \"%s/%s\"", p, ent->d_name);

				p = NULL; /* terminate loop */
				break;
			}
		}
		if(errno)
			die("readdir(%s):", p);

		closedir(d);
	}

	free(path);

	return !!*buf;
}

static char *ucc_readlink(const char *path, char *out, ssize_t outlen)
{
	ssize_t n = readlink(path, out, outlen);

	if(n == -1)
		return NULL;

	if(n == outlen && path[outlen - 1])
		die("readlink(%s), source too large", path);

	return out;
}

static void resolve_argv0_symlink(char buf[], size_t len)
{
	if(strlen(argv0) >= len)
		die("argv[0] too long");
	strcpy(buf, argv0);

	while(ucc_readlink(buf, buf, len));
}

const char *ucc_argv0_path(void)
{
	static char path[4096];

	if(*path)
		return path;

	if(!ucc_readlink("/proc/self/exe", path, sizeof path)){
		/* not linux, try something else */
	}else{
		/* readlink was fine */
		goto out;
	}

	if(strchr(argv0, '/')){
		resolve_argv0_symlink(path, sizeof path);

	}else if(!path_search_self(path, sizeof path)){
		die("can't find ucc in $PATH");
	}

out:
	{
		char *p = strrchr(path, '/');
		if(p)
			*p = '\0';
		return path;
	}
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
	return path_prepend(ucc_argv0_path(), path);
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

const char *cmdpath_type(enum cmdpath_type t)
{
	switch(t){
		case RELATIVE_TO_UCC: return "ucc-relative";
		case USE_PATH: return "$PATH-search";
		case FROM_Bprefix: return "Bprefix-relative";
	}
	return NULL;
}
