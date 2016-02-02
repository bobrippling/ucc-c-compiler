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
#include "str.h"

char **include_paths;

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

char *ucc_where(void)
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
	char *w = ucc_where();
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

		if(wrapper)
			fprintf(stderr, "WRAPPER='%s' ", wrapper);

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
			int nargs = dynarray_count(args);
			int i_in = 0, i_out = 0;
			char **argv;

			/* -wrapper gdb,--args */
			if(wrapper){
				char *p;
				nargs++;
				for(p = wrapper; *p; p++)
					nargs += *p == ',';
			}

			/*
			 * path,
			 * { args }
			 * NULL-term
			 */
			argv = umalloc((2 + nargs) * sizeof *argv);

			/* wrapper */
			if(wrapper){
				char *p, *last;
				for(p = last = wrapper; *p; p++)
					if(*p == ','){
						*p = '\0';
						argv[i_out++] = last;
						last = p + 1;
					}

				if(last != p)
					argv[i_out++] = last;
			}

			argv[i_out++] = local ? actual_path("../", path) : path;

			while(args[i_in])
				argv[i_out++] = args[i_in++];

			argv[i_out++] = NULL;

#ifdef DEBUG
			fprintf(stderr, "%s:\n", *argv);
			for(int i = 0; argv[i]; i++)
				fprintf(stderr, "  [%d] = \"%s\",\n", i, argv[i]);
#endif

			if(wrapper)
				local = 0;

			(local ? execv : execvp)(argv[0], argv);
			die("execv(\"%s\"):", argv[0]);
		}

		default:
		{
			int status, i;
			if(wait(&status) == -1)
				die("wait()");

			if(WIFEXITED(status) && (i = WEXITSTATUS(status)) != 0){
				die("%s returned %d", path, i);
			}else if(WIFSIGNALED(status)){
				int sig = WTERMSIG(status);

				fprintf(stderr, "%s caught signal %d\n", path, sig);

				/* exit with propagating status */
				exit(128 + sig);
			}
		}
	}
}

void execute(char *path, char **args)
{
	runner(0, path, args);
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

void cat(char *fnin, const char *fnout, int append)
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

static void runner_1(int local, char *path, char *in, const char *out, char **args)
{
	char **all = NULL;

	if(args)
		dynarray_add_array(&all, args);

	dynarray_add(&all, (char *)"-o");
	dynarray_add(&all, (char *)out);

	dynarray_add(&all, in);

	runner(local, path, all);

	dynarray_free(char **, all, NULL);
}

void preproc(char *in, const char *out, char **args)
{
	char **all = NULL;
	char **i;

	if(args)
		dynarray_add_array(&all, args);

	for(i = include_paths; i && *i; i++){
		char *this = *i, *inc;
		int f_this = 1;

		if(*this == '/'){
			f_this = 0;
		}else{
			this = actual_path(this, "");
		}

		inc = ustrprintf("-I%s", this);

		dynarray_add(&all, inc);
		if(f_this)
			free(this);
	}

	if(fsystem_cpp)
		runner_1(0, fsystem_cpp, in, out, all);
	else
		runner_1(1, "cpp2/cpp", in, out, all);

	dynarray_free(char **, all, NULL);
}

void compile(char *in, const char *out, char **args)
{
	runner_1(1, "cc1/cc1", in, out, args);
}

void assemble(char *in, const char *out, char **args, char *as)
{
	char **copy = NULL;

	if(args)
		dynarray_add_array(&copy, args);

	runner_1(0, as, in, out, copy);

	dynarray_free(char **, copy, NULL);
}

void link_all(char **objs, const char *out, char **args, char *ld)
{
	char **all = NULL;

	dynarray_add(&all, (char *)"-o");
	dynarray_add(&all, (char *)out);

	dynarray_add_array(&all, objs);

	if(args)
		dynarray_add_array(&all, args);

	runner(0, ld, all);

	dynarray_free(char **, all, NULL);
}
