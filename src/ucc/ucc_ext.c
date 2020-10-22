#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <errno.h>

#include "../util/alloc.h"
#include "../util/dynarray.h"
#include "../util/io.h"

#include "ucc.h"
#include "ucc_ext.h"
#include "ucc_path.h"
#include "umask.h"

#include "str.h"

int time_subcmds;

static int show, noop;

void ucc_ext_cmds_show(int s)
{ show = s; }

void ucc_ext_cmds_noop(int n)
{ noop = n; }

static int runner(struct cmdpath *path, char **args, int return_ec, const char *to_remove)
{
	pid_t pid;
	struct timeval time_start, time_end;

	if(show){
		char *resolved = cmdpath_resolve(path, NULL);
		int i;

		if(wrapper)
			fprintf(stderr, "WRAPPER='%s' ", wrapper);

		fprintf(stderr, "%s ", resolved);
		for(i = 0; args[i]; i++)
			fprintf(stderr, "%s ", args[i]);

		fputc('\n', stderr);

		free(resolved);
	}

	if(noop)
		return 0;

	if(time_subcmds && gettimeofday(&time_start, NULL) < 0)
		fprintf(stderr, "gettimeofday(): %s\n", strerror(errno));

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

			umask(orig_umask);

			(*execfn)(argv[0], argv);
			die("execv(\"%s\"):", argv[0]);
		}

		default:
		{
			int status, i;
			if(wait(&status) == -1)
				die("wait()");

			if(time_subcmds && gettimeofday(&time_end, NULL) < 0)
				fprintf(stderr, "gettimeofday(): %s\n", strerror(errno));

			if(WIFEXITED(status) && (i = WEXITSTATUS(status)) != 0){
				if(to_remove)
					remove(to_remove);
				if(!return_ec)
					die("%s returned %d", path->path, i);
			}else if(WIFSIGNALED(status)){
				int sig = WTERMSIG(status);

				fprintf(stderr, "%s caught signal %d\n", path->path, sig);

				if(to_remove)
					remove(to_remove);

				/* exit with propagating status */
				exit(128 + sig);
			}

			if(time_subcmds){
				time_t secdiff = time_end.tv_sec - time_start.tv_sec;
				suseconds_t usecdiff = time_end.tv_usec - time_start.tv_usec;

				if(usecdiff < 0){
					secdiff--;
					usecdiff += 1000000L;
				}

				printf("# %s %ld.%06ld\n", path->path, (long)secdiff, (long)usecdiff);
			}

			return i;
		}
	}
}

void execute(char *path, char **args)
{
	struct cmdpath cmdpath;

	cmdpath.path = path;
	cmdpath.type = USE_PATH;

	runner(&cmdpath, args, 0, NULL);
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

	if(!strcmp(old, new))
		return;

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
	runner(&shpath, fixed, 0, new);

	free(cmd);
}

void cat_fnames(char *fnin, const char *fnout, int append)
{
	FILE *in, *out;

	if(show){
		fprintf(stderr, "cat %s%s%s\n",
				fnin,
				fnout && append ? " >>" : "",
				fnout ? fnout : "");
	}

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

	if(cat(in, out))
		die("write():");

	if(fclose(in))
		die("close():");

	if(fnout && fclose(out) == EOF)
		die("close():");
}

static int runner_single_arg(
		struct cmdpath *path,
		char *in,
		const char *out,
		char **args,
		int return_ec)
{
	int ret;
	char **all = NULL;

	if(args)
		dynarray_add_array(&all, args);

	dynarray_add(&all, (char *)"-o");
	dynarray_add(&all, (char *)out);

	dynarray_add(&all, in);

	ret = runner(path, all, return_ec, out);

	dynarray_free(char **, all, NULL);

	return ret;
}

int preproc(char *in, const char *out, char **args, int return_ec)
{
	int ret;
	char **all = NULL;
	struct cmdpath pp_path;

	if(args)
		dynarray_add_array(&all, args);

	if(binpath_cpp){
		pp_path.type = USE_PATH;
		pp_path.path = binpath_cpp;
	}else{
		cmdpath_initrelative(&pp_path, "cpp", "../cpp2/cpp");
	}

	ret = runner_single_arg(&pp_path, in, out, all, return_ec);

	dynarray_free(char **, all, NULL);

	return ret;
}

int compile(char *in, const char *out, char **args, int return_ec)
{
	struct cmdpath cc1path;

	cmdpath_initrelative(&cc1path, "cc1", "../cc1/cc1");

	return runner_single_arg(&cc1path, in, out, args, return_ec);
}

void assemble(char *in, const char *out, char **args, const char *as)
{
	char **copy = NULL;
	struct cmdpath aspath;

	if(args)
		dynarray_add_array(&copy, args);

	aspath.path = as;
	aspath.type = USE_PATH;
	runner_single_arg(&aspath, in, out, copy, 0);

	dynarray_free(char **, copy, NULL);
}

void link_all(char **objs, const char *out, char **args, const char *ld)
{
	char **all = NULL;
	struct cmdpath ldpath;

	dynarray_add(&all, (char *)"-o");
	dynarray_add(&all, (char *)out);

	dynarray_add_array(&all, objs);

	if(args)
		dynarray_add_array(&all, args);

	ldpath.path = ld;
	ldpath.type = USE_PATH;

	runner(&ldpath, all, 0, out);

	dynarray_free(char **, all, NULL);
}
