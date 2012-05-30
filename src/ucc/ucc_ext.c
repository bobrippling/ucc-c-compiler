#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "ucc_ext.h"
#include "ucc.h"
#include "../util/alloc.h"
#include "../util/dynarray.h"

static char *
where()
{
	static char where[1024];

	if(!where[0]){
		char *p;
		ssize_t nb;

		if((nb = readlink(argv0, where, sizeof where)) == -1)
			snprintf(where, sizeof where, "%s", argv0);
		else
			where[nb] = '\0';

		/* dirname */
		p = strrchr(where, '/');
		if(p)
			*++p = '\0';
	}

	return where;
}

char *actual_path(const char *prefix, const char *path)
{
	char *w = where();
	char *buf;

	buf = umalloc(strlen(w) + strlen(prefix) + strlen(path) + 2);

	sprintf(buf, "%s/%s%s", w, prefix, path);

	return buf;
}

static void runner(int local, char *path, char **args)
{
	pid_t pid = fork();

	switch(pid){
		case -1:
			die("fork():");

		case 0:
		{
			const int nargs = dynarray_count((void **)args);
			int i;
			char **argv;

			/*
			 * path,
			 * { args }
			 * NULL-term
			 */
			argv = umalloc((2 + nargs) * sizeof *argv);

			if(local)
				argv[0] = actual_path("../", path);
			else
				argv[0] = path;

			for(i = 0; args[i]; i++)
				argv[i + 1] = args[i];

			argv[++i] = NULL;

#ifdef DEBUG
			for(i = 0; argv[i]; i++)
				fprintf(stderr, "  [%d] = \"%s\",\n", i, argv[i]);
#endif

			(local ? execv : execvp)(argv[0], argv);
			die("execv():");
		}

		default:
		{
			int status;
			if(wait(&status) == -1)
				die("wait()");

			if(!WIFEXITED(status) || WEXITSTATUS(status) != 0)
				die("%s returned %d", path, status);
		}
	}
}

static void runner_1(int local, char *path, char *in, char *out, char **args)
{
	char **all = NULL;

	if(args)
		dynarray_add_array((void ***)&all, (void **)args);

	dynarray_add((void ***)&all, "-o");
	dynarray_add((void ***)&all, out);

	dynarray_add((void ***)&all, in);

	runner(local, path, all);

	dynarray_free((void ***)&all, NULL);
}

void preproc(char *in, char *out, char **args)
{
	runner_1(1, "cpp2/cpp", in, out, args);
}

void compile(char *in, char *out, char **args)
{
	runner_1(1, "cc1/cc1", in, out, args);
}

void assemble(char *in, char *out, char **args)
{
	runner_1(0, "nasm", in, out, args);
}

void link_all(char **objs, char *out, char **args)
{
	char **all = NULL;

	dynarray_add((void ***)&all, "-o");
	dynarray_add((void ***)&all, out);

	dynarray_add_array((void ***)&all, (void **)objs); /* TODO: order is important... */

	if(args)
		dynarray_add_array((void ***)&all, (void **)args);

	runner(0, "ld", all);

	dynarray_free((void ***)&all, NULL);
}
