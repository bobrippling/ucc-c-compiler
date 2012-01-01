#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
/* dirname */
#include <libgen.h>
/* chdir, getcwd */
#include <unistd.h>
/* open */
#include <fcntl.h>

#include "pp.h"
#include "../util/util.h"
#include "../util/alloc.h"
#include "../util/platform.h"
#include "../util/dynarray.h"
#include "str.h"

#define newline() outline(p, "")

#define VERBOSE(...) do{ if(pp_verbose) fprintf(stderr, ">> " __VA_ARGS__); }while(0)

struct def
{
	char *name, *val;
	char **args;
	int is_func;
	struct def *next;
};


static struct def *defs = NULL;
static const char **dirs = NULL;
static FILE  *devnull;
static int    pp_verbose = 0;
static int counter; /* __COUNTER__ */

int current_line = 0, current_chr = 0;
const char *current_fname = NULL;

extern int make_rules;

static void ppdie(struct pp *p, const char *fmt, ...)
{
	va_list l;
	struct where where;

	if(p){
		where.fname = p->fname;
		where.line  = p->nline;
		where.chr   = -1;
	}

	if(fmt){
		fputs("cpp: ", stderr);

		va_start(l, fmt);
		vdie(&where, fmt, l);
	}else{
		exit(1);
	}
}

void adddir(char *d)
{
	dynarray_add((void ***)&dirs, d);

	VERBOSE("adddir(\"%s\")\n", d);
}

void undef(const char *s)
{
	struct def *d, *prev;

	for(prev = NULL, d = defs; d; prev = d, d = d->next)
		if(!strcmp(d->name, s)){
			if(prev){
				prev->next = d->next;
			}else{
				defs = d->next;
			}
			free(d->name);
			free(d->val);
			free(d);
			break;
		}
}

static struct def *getdef(const char *s)
{
	struct def *d;

	for(d = defs; d; d = d->next)
		if(!strcmp(d->name, s))
			return d;
	return NULL;
}

struct def *adddef(char *n, char *v)
{
	/* FIXME: check for substring v in n */
	struct def *d;

	if((d = getdef(n))){
		fprintf(stderr, "warning: \"%s\" redefined\n", n);
		free(d->val);
		d->val = ustrdup(v);
		return d;
	}

	d = umalloc(sizeof *d);

	d->name = n;
	d->val  = v;

	d->next = defs;
	defs = d;

	VERBOSE("adddef(\"%s\", \"%s\")\n", n, v);

	return d;
}

struct def *addmacro(char *mname, char **args, char *rest)
{
	struct def *d = adddef(mname, rest);

	d->is_func = 1;
	d->args = args;

	if(pp_verbose){
		int i;
		fprintf(stderr, ">> macro %s\n", mname);
		for(i = 0; args && args[i]; i++)
			fprintf(stderr, ">> macro_arg[%d] = %s\n", i, args[i]);
		fprintf(stderr, ">> rest %s\n", rest);
	}

	return d;
}

int in_quote(const char *line, const char *pos)
{
	const char *p;
	int in = 0;
	for(p = line; p < pos; p++)
		if(*p == '"' && (p > line ? p[-1] != '\\' : 1))
			in = !in; /* this is broken for "\\" */
	return in;
}

static void substitutedef(struct pp *p, char **line)
{
	struct def *d;
	char *pos = *line;

	for(d = defs; d; d = d->next){
		/* TODO: word-sep */
restart:
		while(pos && (pos = findword(pos, d->name))){
			char nbuf[16];
			char *post;
			char *val;
			int freeval = 0;

			/* dirty hack */
			if(in_quote(*line, pos)){
				fprintf(stderr, "%s in quote\n", pos);
				pos++;
				goto restart;
			}

			if(pp_verbose)
				fprintf(stderr, "replacing %s\n", pos);

			post = pos + strlen(d->name);

			if(!strcmp(d->name, "__LINE__")){
				snprintf(nbuf, sizeof nbuf, "%d", p->nline);
				val = nbuf;
			}else if(!strcmp(d->name, "__FILE__")){
				val = ustrprintf("\"%s\"", p->fname);
				freeval = 1;
			}else if(!strcmp(d->name, "__COUNTER__")){
				snprintf(nbuf, sizeof nbuf, "%d", counter++);
				val = nbuf;
			}else{
				val = d->val;
			}

			if(d->is_func){
				char **args = NULL;
				int arg_got, arg_expected;
				int i;

				{
					char *arg_start, *arg_fin;
					char *dup, *cur;
					int len;

					arg_start = post;
					arg_fin = strchr(arg_start - 1, ')'); /* FIXME: nesting */

					if(!arg_fin)
						ppdie(p, "no close paren for macro call (%s)", *line);

					arg_fin--;
					len = arg_fin - arg_start + 1;
					if(len > 0){
						dup = umalloc(len);
						strncpy(dup, arg_start + 1, len - 1);
						dup[len-1] = '\0';

						for(cur = strtok(dup, ","); cur; cur = strtok(NULL, ","))
							dynarray_add((void ***)&args, ustrdup(cur));
						free(dup);
					}
					post = arg_fin + 2;
				}

				arg_got      = dynarray_count((void **)args);
				arg_expected = dynarray_count((void **)d->args);

				if(arg_expected != arg_got){
					if(pp_verbose)
						for(i = 0; i < arg_got; i++)
							fprintf(stderr, "arg[%d] = \"%s\"\n", i, args[i]);

					ppdie(p, "mismatching argument counts for macro (got %d, expected %d)",
							arg_got, arg_expected);
				}

#ifdef MACRO_FUNC_DEBUG
				for(i = 0; args && args[i]; i++)
					fprintf(stderr, "args[%d] = \"%s\"\n", i, args[i]);
#endif

				val = ustrdup(val);
				freeval = 1;

				for(i = 0; args && args[i]; i++){
					char *new;

#ifdef MACRO_FUNC_DEBUG
					fprintf(stderr, "before = \"%s\", s/%s/%s/g, after = \"",
							val, d->args[i], args[i]);
#endif

					new = strreplace(val, d->args[i], args[i]);

#ifdef MACRO_FUNC_DEBUG
					fprintf(stderr, "%s\"\n", new);
#endif

					free(val);
					val = new;
				}

				dynarray_free((void ***)&args, free);
			}

			{
				char *new;

				*pos = post[-1] = '\0';

				new = ustrprintf("%s%s%s", *line, val, post);
				free(*line);
				*line = new;
			}
			d = defs;
			pos = *line;

			if(freeval)
				free(val);
			/*
			 * recursive defs - could be infinite loop, but oh well,
			 * run out of memory eventually
			 */
		}
	}
}

static void freedefs()
{
	struct def *d;
	while(defs){
		d = defs;
		defs = defs->next;

		if(d->args)
			dynarray_free((void ***)&d->args, free);

		free(d->name);
		free(d->val);

		free(d);
	}
}

static void outline(struct pp *p, const char *line)
{
	if(!make_rules)
		fprintf(p->out, "%s\n", line);
}

static void define(struct pp *pp, char **argv)
{
	if(strchr(argv[1], '(')){
		char **args = NULL;
		char *mname = NULL;
		char *line;
		char *p, *iter, *fin;

		if(!argv[2]){
			/* #define x() */
			addmacro(ustrdup(argv[1]), NULL, ustrdup(""));
			return;
		}

		line = umalloc(strlen(argv[1]) + strlen(argv[2]) + 1);
		sprintf(line, "%s%s", argv[1], argv[2]);

		p   = strchr(line, '(');
		fin = strchr(p,    ')');

		if(!fin)
			ppdie(pp, "no closing paren for macro definition");

		*fin = '\0';

		for(iter = argv[1]; *iter; iter++)
			if(*iter == '(')
				break;
			else if(!isalpha(*iter) && *iter != '_')
				ppdie(pp, "invalid macro name %s (char %c)", argv[1], *iter);

		*iter = '\0';
		mname = ustrdup(argv[1]);
		*iter = '(';

#define SPACE_WALK() \
			while(isspace(*iter)) \
				iter++

		for(iter = p + 1; *iter; iter++){
			SPACE_WALK();

			if(isalpha(*iter)){
				char *start = iter;
				char save;

				while(isalnum(*iter))
					iter++;

				save = *iter;
				*iter = '\0';
				dynarray_add((void ***)&args, ustrdup(start));
				*iter = save;

				SPACE_WALK();

				if(*iter == ',')
					continue;
				else if(iter == fin)
					break;
				else
					goto invalid_ch;
			}else{
invalid_ch:
				ppdie(pp, "invalid macro (at char #%d)", *iter);
			}
		}

		addmacro(mname, args, ustrdup(fin+1));

		free(line);
	}else{
		ADDDEF(argv[1], argv[2] ? argv[2] : "");
	}
}

static int pp(struct pp *p, int skip, int need_chdir)
{
#define RET(x) do{ free(linealloc); ret = x; goto fin; }while(0)
	int curwdfd;
	int ret;

	/* save for "cd -" */
	if(need_chdir){
		char *wd;

		curwdfd = open(".", O_RDONLY);
		if(curwdfd == -1)
			ppdie(p, "open(\".\"): %s", strerror(errno));

		/* make sure everything is relative to the file */
		wd = udirname(p->fname);
		if(chdir(wd))
			ppdie(p, "chdir(\"%s\"): %s (for %s)", wd, strerror(errno), p->fname);
		free(wd);
	}else{
		curwdfd = -1;
	}


	do{
		char *linealloc;
		char *line, *nl;

		line = linealloc = fline(p->in);
		p->nline++;

		if(!line){
			if(feof(p->in))
				/* normal exit here */
				RET(PROC_EOF);

			ppdie(p, "read(): %s", strerror(errno));
		}

		if((nl = strchr(line, '\n')))
			*nl = '\0';

		if(*line == '#'){
			char *argv[3] = { NULL };
			char *s, *last;
			int i, argc;
			int flag = 0;

			line++;
			while(isspace(*line))
				line++;

			if(!*line)
				ppdie(p, "no preprocessor command");

			for(i = 0, s = last = line; *s; s++)
				if(isspace(*s)){
					argv[i++] = last;
					if(i == 3)
						break;
					*s++ = '\0';
					while(isspace(*s))
						s++;
					last = s;
				}
			if(i < 3 && last != s)
				argv[i++] = last;
			argc = i;

			if(!strcmp(argv[0], "include")){
				if(argc != 2)
					ppdie(p, "include takes one argument");

				if(!skip){
					struct pp pp2;
					FILE *inc;
					char *base, *path;
					int i, found = 0;
					int incchar;

					memset(&pp2, 0, sizeof pp2);

					base = line + 8;
					while(isspace(*base))
						base++;

					switch(*base){
						case '"':
							incchar = '"';
							break;

						case '<':
							incchar = '>';
							break;

						default:
							ppdie(p, "invalid include char %c (%s)", *base, line);
					}

					for(path = ++base; *path; path++)
						if(*path == incchar){
							*path = '\0';
							found = 1;
							break;
						}

					if(!found)
						ppdie(p, "no terminating '%c' for include \"%s\"", incchar, line);

					found = 0;

					if(incchar == '>'){
						for(i = 0; dirs[i]; i++){
							path = ustrprintf("%s/%s", dirs[i], base);

							inc = fopen(path, "r");

							if(inc){
								found = 1;
								break;
							}

							free(path);
						}

						if(!found){
							char pwd[1024];
							if(pp_verbose){
								getcwd(pwd, sizeof pwd);

								for(i = 0; dirs[i]; i++)
									fprintf(stderr, "tried %s/%s\n", dirs[i], base);
							}

							ppdie(p, "can't find include file <%s>%s%s%s", base,
									pp_verbose ? " (pwd " : "",
									pp_verbose ? pwd : "",
									pp_verbose ? ")" : ""
									);
						}

					}else{
						path = ustrdup(base);
						inc = fopen(path, "r");
						if(!inc){
							char pwd[1024];
							if(pp_verbose)
								getcwd(pwd, sizeof pwd);

							ppdie(p, "can't open \"%s\": %s%s%s%s", path, strerror(errno),
									pp_verbose ? " (pwd " : "",
									pp_verbose ? pwd : "",
									pp_verbose ? ")" : ""
									);
						}
					}

					if(incchar == '"' && make_rules)
						dynarray_add((void ***)&p->deps, path);

					VERBOSE("including %s\n", path);
					pp2.fname = path;

					pp2.in  = inc;
					pp2.out = p->out;

					switch(pp(&pp2, 0, 1)){
						case PROC_EOF:
							break;

						case PROC_ELSE:
						case PROC_ENDIF:
						case PROC_ERR:
							ppdie(p, "eof expected from including \"%s\"", pp2.fname);
					}

					VERBOSE("included %s\n", path);

					fclose(inc);
					free(path); /* pp2.fname */
				}
			}else if(!strcmp(argv[0], "define")){
				if(argc < 2)
					ppdie(p, "define takes at least one argument");

				newline();
				if(!skip)
					define(p, argv);

			}else if(!strcmp(argv[0], "undef")){
				if(argc != 2)
					ppdie(p, "undef takes a single argument");

				newline();
				undef(argv[1]);

			}else if(!strcmp(argv[0], "ifdef") || (flag = !strcmp(argv[0], "ifndef"))){
				struct pp arg;
				int gotdef;
				int ret;

				newline();

				if(argc != 2)
					ppdie(p, "if%sdef takes one argument", flag ? "n" : "");

				VERBOSE("if%sdef %s\n", flag ? "n" : "", argv[1]);

				if(skip){
					gotdef = 0;
				}else{
					gotdef = !!getdef(argv[1]);

					if(flag)
						gotdef = !gotdef;
				}

				memcpy(&arg, p, sizeof arg);
				arg.out = devnull;

				ret = pp(gotdef ? p : &arg, skip || !gotdef, 0);

				switch(ret){
					case PROC_ELSE:
						ret = pp(gotdef ? &arg : p, skip || gotdef, 0);
						if(ret != PROC_ENDIF)
							ppdie(p, "endif expected");
					case PROC_ENDIF:
						break;

					case PROC_EOF:
						ppdie(p, "eof unexpected");
				}

			}else if(!strcmp(argv[0], "else")){
				newline();
				if(argc != 1)
					ppdie(p, "invalid #else");

				VERBOSE("else\n");

				RET(PROC_ELSE);
			}else if(!strcmp(argv[0], "endif")){
				if(argc != 1)
					ppdie(p, "invalid #endif");

				VERBOSE("endif\n");

				newline();
				RET(PROC_ENDIF);
			}else if(!strcmp(argv[0], "warning") || (flag = !strcmp(argv[0], "error"))){
				const char *mode = flag ? "error" : "warning";
				int i;
				if(argc < 2)
					ppdie(p, "#%s needs at least one arg", mode);
				fprintf(stderr, "%s: ", mode);
				for(i = 1; i < argc; i++)
					fprintf(stderr, "%s%c", argv[i], i == argc - 1 ? '\n' : ' ');

				newline();
				if(flag)
					ppdie(p, NULL);
			}else{
				ppdie(p, "\"%s\" unexpected", line);
			}
		}else{
			substitutedef(p, &linealloc);
			outline(p, linealloc);
		}
		free(linealloc);
	}while(1);
#undef RET
fin:
	if(curwdfd != -1){
		if(fchdir(curwdfd) == -1)
			ppdie(p, "chdir(-): %s", strerror(errno));
		close(curwdfd);
	}

	if(make_rules){
		char **i;
		char *tmp;

		/*
		 * if we're handling "tim.c", we should output this:
		 * tim.o: tim.c [includes]
		 */

		printf("%s: %s", tmp = ext_replace(p->fname, "o"), p->fname);
		free(tmp);

		for(i = p->deps; i && *i; i++){
			printf(" %s", *i);
		}
		putchar('\n');
	}

	return ret;
}

void def_defs(void)
{
	ADDDEF("__FILE__", "");
	ADDDEF("__LINE__", "");
	ADDDEF("__COUNTER__", "");
}

enum proc_ret preprocess(struct pp *p, int verbose)
{
	int ret;

	pp_verbose = verbose;

	devnull = fopen("/dev/null", "w");
	if(!devnull){
		perror("open /dev/null");
		return PROC_ERR;
	}

	def_defs();

	if(verbose){
		int i;
		for(i = 0; dirs[i]; i++)
			fprintf(stderr, ">> include dir \"%s\"\n", dirs[i]);
	}

	ret = pp(p, 0, 1);

	if(make_rules){
		char **i;
		char *tmp;

		/*
		 * if we're handling "tim.c", we should output this:
		 * tim.o: tim.c [includes]
		 */

		printf("%s: %s", tmp = ext_replace(p->fname, "o"), p->fname);
		free(tmp);

		for(i = p->deps; i && *i; i++){
			printf(" %s", *i);
		}
		putchar('\n');
	}

	freedefs();

	fclose(devnull);
	devnull = NULL;

	return ret != PROC_EOF;
}
