#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdarg.h>

#include "ucc_ext.h"
#include "ucc.h"
#include "../util/alloc.h"
#include "../util/dynarray.h"
#include "cfg.h"

#ifndef UCC_AS
# error "ucc needs reconfiguring"
#endif

static int show, noop;

void ucc_ext_cmds_show(int s)
{ show = s; }

void ucc_ext_cmds_noop(int n)
{ noop = n; }


static void
bname(char *path)
{
	char *p = strrchr(path, '/');
	if(p)
		p[1] = '\0';
}

static char *
where()
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
	pid_t pid;

	if(show){
		int i;

		fprintf(stderr, "%s ", path);
		for(i = 0; args[i]; i++)
			fprintf(stderr, "%s ", args[i]);
		fputc('\n', stderr);
	}

	if(noop)
		return;


	/* if this were to be vfork, all the code in case-0 would need to be done in the parent */
	pid = fork();

	switch(pid){
		case -1:
			die("fork():");

		case 0:
		{
			const int nargs = dynarray_count(args);
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
			fprintf(stderr, "%s:\n", *argv);
			for(i = 0; argv[i]; i++)
				fprintf(stderr, "  [%d] = \"%s\",\n", i, argv[i]);
#endif

			(local ? execv : execvp)(argv[0], argv);
			die("execv():");
		}

		default:
		{
			int status, i;
			if(wait(&status) == -1)
				die("wait()");

			if(WIFEXITED(status) && (i = WEXITSTATUS(status)) != 0)
				die("%s returned %d", path, i);
			else if(WIFSIGNALED(status))
				die("%s caught signal %d", path, WTERMSIG(status));
		}
	}
}

void rename_or_move(char *old, char *new)
{
	/* don't move, cat instead, since we can't move to /dev/null, for e.g. */
	int len, i;
	char *cmd, *p;
	char *args[] = {
		"cat", old, ">", new, NULL
	};
	char *fixed[3];

	for(i = 0, len = 1; args[i]; i++)
		len += strlen(args[i]) + 1; /* space */

	p = cmd = umalloc(len);

	for(i = 0; args[i]; i++)
		p += sprintf(p, "%s ", args[i]);

	fixed[0] = "-c";
	fixed[1] = cmd;
	fixed[2] = NULL;

	runner(0, "sh", fixed);

	free(cmd);
}

void cat(char *fnin, char *fnout, int append)
{
	FILE *in, *out;
	char buf[1024];
	size_t n;

	if(show)
		fprintf(stderr, "cat %s >%s %s\n", fnin, append ? ">" : "", fnout ? fnout : "<stdout>");
	if(noop)
		return;

	in  = fopen(fnin,  "r");
	if(!in)
		die("open %s:", fnin);

	if(fnout){
		out = fopen(fnout, append ? "a" : "w");
		if(!out)
			die("open %s:", fnout);
	}else{
		out = stdout;
	}

	while((n = fread(buf, sizeof *buf, sizeof buf, in)) > 0)
		if(fwrite(buf, sizeof *buf, n, out) != n)
			die("write():");

	if(ferror(in))
		die("read():");

	fclose(in);

	if(fnout && fclose(out) == EOF)
		die("close():");
}

static void runner_1(int local, char *path, char *in, char *out, char **args)
{
	char **all = NULL;

	if(args)
		dynarray_add_array(&all, args);

	dynarray_add(&all, (char *)"-o");
	dynarray_add(&all, out);

	dynarray_add(&all, in);

	runner(local, path, all);

	dynarray_free(&all, NULL);
}

void preproc(char *in, char *out, char **args)
{
	char **all = NULL;
	char *inc_path;
	char *inc;

	if(args)
		dynarray_add_array(&all, args);

	inc_path = actual_path("../../lib/", "");
	inc = ustrprintf("-I%s", inc_path);

	dynarray_add(&all, inc);

	runner_1(1, "cpp2/cpp", in, out, all);

	free(inc);
	free(inc_path);
	dynarray_free(&all, NULL);
}

void compile(char *in, char *out, char **args)
{
	runner_1(1, "cc1/cc1", in, out, args);
}

void assemble(char *in, char *out, char **args)
{
	char **copy = NULL;

	if(args)
		dynarray_add_array(&copy, args);

	runner_1(0, UCC_AS, in, out, copy);

	dynarray_free(&copy, NULL);
}

void link_all(char **objs, char *out, char **args)
{
	char **all = NULL;
	char *tok, *dup;

	dynarray_add(&all, (char *)"-o");
	dynarray_add(&all, out);

	dup = ustrdup(UCC_LDFLAGS);

	for(tok = strtok(dup, " "); tok; tok = strtok(NULL, " "))
		dynarray_add(&all, tok);

	dynarray_add_array(&all, objs);

	/* TODO: order is important - can't just group all objs at the end, etc */

	if(args)
		dynarray_add_array(&all, args);

	runner(0, "ld", all);

	dynarray_free(&all, NULL);
	free(dup);
}
