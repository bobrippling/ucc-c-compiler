#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#include "../util/alloc.h"
#include "../util/dynarray.h"

#include "ucc.h"
#include "ucc_ext.h"
#include "ucc_path.h"
#include "specs.h"

#include "str.h"

#include "cfg.h"


#ifndef UCC_AS
# error "ucc needs reconfiguring"
#endif

char **include_paths;

static int show, show_path, noop;

void ucc_ext_cmds_show(int s)
{ show = s; }

void ucc_ext_cmds_noop(int n)
{ noop = n; }

void ucc_ext_cmds_show_path(int p)
{ show_path = p; }

static void runner(struct cmdpath *path, char **args)
{
	pid_t pid;

	if(show){
		int i;

		if(show_path)
			fprintf(stderr, "PATH_TYPE='%s'\n", cmdpath_type(path->type));

		if(wrapper)
			fprintf(stderr, "WRAPPER='%s' ", wrapper);

		fprintf(stderr, "%s ", path->path);
		for(i = 0; args[i]; i++)
			fprintf(stderr, "%s ", args[i]);
		fputc('\n', stderr);

		if(show_path){
			char *res = cmdpath_resolve(path, NULL);
			fprintf(stderr, "RESOLVED_PATH='%s'\n", res);
			free(res);
		}
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
			cmdpath_exec_fn *execfn;

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

			argv[i_out++] = cmdpath_resolve(path, &execfn);

			while(args[i_in])
				argv[i_out++] = args[i_in++];

			argv[i_out++] = NULL;

#ifdef DEBUG
			fprintf(stderr, "%s:\n", *argv);
			for(int i = 0; argv[i]; i++)
				fprintf(stderr, "  [%d] = \"%s\",\n", i, argv[i]);
#endif

			(*execfn)(argv[0], argv);
			die("execv(\"%s\"):", argv[0]);
		}

		default:
		{
			int status, i;
			if(wait(&status) == -1)
				die("wait()");

			if(WIFEXITED(status) && (i = WEXITSTATUS(status)) != 0)
				die("%s returned %d", path->path, i);
			else if(WIFSIGNALED(status))
				die("%s caught signal %d", path->path, WTERMSIG(status));
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
	struct cmdpath shpath;

	for(i = 0, len = 1; args[i]; i++)
		len += strlen(args[i]) + 1; /* space */

	p = cmd = umalloc(len);

	for(i = 0; args[i]; i++)
		p += sprintf(p, "%s ", args[i]);

	fixed[0] = "-c";
	fixed[1] = cmd;
	fixed[2] = NULL;

	shpath.path = "sh";
	shpath.type = USE_PATH;
	runner(&shpath, fixed);

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

static void runner_single_arg(
		struct cmdpath *path,
		char *in, char *out,
		char **args)
{
	char **all = NULL;

	if(args)
		dynarray_add_array(&all, args);

	dynarray_add(&all, (char *)"-o");
	dynarray_add(&all, out);

	dynarray_add(&all, in);

	runner(path, all);

	dynarray_free(char **, &all, NULL);
}

static void make_cmdpath(
		struct cmdpath *p,
		const char *bprefix, const char *ucc_relative)
{
	if(Bprefix){
		p->path = bprefix;
		p->type = FROM_Bprefix;
	}else{
		p->path = ucc_relative;
		p->type = RELATIVE_TO_UCC;
	}
}

void preproc(char *in, char *out, char **args)
{
	char **all = NULL;
	char **i;
	struct cmdpath pp_path;

	if(args)
		dynarray_add_array(&all, args);

	for(i = include_paths; i && *i; i++){
		char *this = *i, *inc;
		int f_this = 1;

		if(*this == '/'){
			f_this = 0;
		}else{
			this = path_prepend_relative(this);
		}

		inc = ustrprintf("-I%s", this);

		dynarray_add(&all, inc);
		if(f_this)
			free(this);
	}

#if 0
	make_cmdpath(&pp_path, "cpp", specs.cpp);
#else
	/* use the system preproc. for now */
	pp_path.type = USE_PATH;
	pp_path.path = "cpp";
#endif

	runner_single_arg(&pp_path, in, out, all);

	dynarray_free(char **, &all, NULL);
}

void compile(char *in, char *out, char **args)
{
	struct cmdpath cc1path;

	make_cmdpath(&cc1path, "cc1", specs.cc1);

	runner_single_arg(&cc1path, in, out, args);
}

void assemble(char *in, char *out, char **args)
{
	char **copy = NULL;
	struct cmdpath aspath;

	if(args)
		dynarray_add_array(&copy, args);

	aspath.path = "as";
	aspath.type = USE_PATH;
	runner_single_arg(&aspath, in, out, copy);

	dynarray_free(char **, &copy, NULL);
}

void link_all(char **objs, char *out, char **args)
{
	char **all = NULL;
	struct cmdpath ldpath;

	dynarray_add(&all, (char *)"-o");
	dynarray_add(&all, out);

	/* note: order is important - can't just group all objs at the end
	 * this is handled in configure
	 */

	dynarray_add_tmparray(&all, strsplit(UCC_LDFLAGS, " "));

	dynarray_add_array(&all, objs);

	if(args)
		dynarray_add_array(&all, args);

	ldpath.path = "ld";
	ldpath.type = USE_PATH;

	runner(&ldpath, all);

	dynarray_free(char **, &all, NULL);
}
