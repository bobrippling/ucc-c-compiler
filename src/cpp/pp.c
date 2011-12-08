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

#define newline() outline(p, "")

struct def
{
	char *name, *val;
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

	if(pp_verbose)
		fprintf(stderr, "adddir(\"%s\")\n", d);
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

static char *strdup_printf(const char *fmt, ...)
{
	va_list l;
	char *buf = NULL;
	int len = 8, ret;

	do{
		len *= 2;
		buf = urealloc(buf, len);
		va_start(l, fmt);
		ret = vsnprintf(buf, len, fmt, l);
		va_end(l);
	}while(ret >= len);

	return buf;
}

static struct def *getdef(const char *s)
{
	struct def *d;

	for(d = defs; d; d = d->next)
		if(!strcmp(d->name, s))
			return d;
	return NULL;
}

void adddef(const char *n, const char *v)
{
	/* FIXME: check for substring v in n */
	char *n2, *v2;
	struct def *d;

	if((d = getdef(n))){
		fprintf(stderr, "warning: \"%s\" redefined\n", n);
		free(d->val);
		d->val = ustrdup(v);
		return;
	}

	d = umalloc(sizeof *d);

	n2 = ustrdup(n);
	v2 = ustrdup(v);

	strcpy(n2, n);
	strcpy(v2, v);

	d->name = n2;
	d->val  = v2;

	d->next = defs;
	defs = d;

	if(pp_verbose)
		fprintf(stderr, "adddef(\"%s\", \"%s\")\n", n, v);
}

static void substitutedef(struct pp *p, char **line)
{
	struct def *d;
	char *pos;

	for(d = defs; d; d = d->next){
		while((pos = strstr(*line, d->name))){
			char nbuf[16];
			char *const post = pos + strlen(d->name);
			char *new;
			const char *val;

			*pos = post[-1] = '\0';

			if(!strcmp(d->name, "__LINE__")){
				snprintf(nbuf, sizeof nbuf, "%d", p->nline);
				val = nbuf;
			}else if(!strcmp(d->name, "__FILE__")){
				val = p->fname; /* FIXME: quote */
			}else if(!strcmp(d->name, "__COUNTER__")){
				snprintf(nbuf, sizeof nbuf, "%d", counter++);
				val = nbuf;
			}else{
				val = d->val;
			}

			new = strdup_printf("%s%s%s", *line, val, post);
			free(*line);
			*line = new;
			d = defs;
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

static int pp(struct pp *p, int skip, int need_chdir)
{
#define RET(x) do{ ret = x; goto fin; }while(0)
	int curwdfd;
	int ret;
	char *line, *nl;
	char *wd;

	/* save for "cd -" */
	if(need_chdir){
		curwdfd = open(".", O_RDONLY);
		if(curwdfd == -1)
			ppdie(p, "open(\".\"): %s", strerror(errno));

		/* make sure everything is relative to the file */
		wd = udirname(p->fname);
		if(chdir(wd))
			ppdie(p, "chdir(\"%s\"): %s (for %s)", wd, strerror(errno), p->fname);
	}else{
		curwdfd = -1;
	}


	do{
		int flag;

		line = fline(p->in);
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

			for(i = 0, last = s = line + 1; *s; s++)
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

					base = line + 9;
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
							ppdie(p, "invalid include char %c", *base);
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
							path = strdup_printf("%s/%s", dirs[i], base);

							inc = fopen(path, "r");

							if(inc){
								found = 1;
								break;
							}

							free(path);
						}

						if(!found){
							char pwd[1024];
							if(pp_verbose)
								getcwd(pwd, sizeof pwd);
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

					if(pp_verbose)
						fprintf(stderr, "including %s\n", path);
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

					if(pp_verbose)
						fprintf(stderr, "included %s\n", path);

					fclose(inc);
					free(path); /* pp2.fname */
				}
			}else if(!strcmp(argv[0], "define")){
				if(argc < 2)
					ppdie(p, "define takes at least one argument");

				newline();
				if(!skip)
					adddef(argv[1], argv[2] ? argv[2] : "");

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
					ppdie(p, "ifdef takes one argument");

				if(flag)
					skip = !skip;

				if(skip)
					gotdef = 0;
				else
					gotdef = !!getdef(argv[1]);

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

				free(line);
				RET(PROC_ELSE);
			}else if(!strcmp(argv[0], "endif")){
				if(argc != 1)
					ppdie(p, "invalid #endif");

				newline();
				free(line);
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
			substitutedef(p, &line);
			outline(p, line);
		}
		free(line);
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
	char buf[8];
	char regc;
	int b64;

	b64 = platform_type() == PLATFORM_64;

	if(b64){
		adddef("UCC_64_BIT", "");
		regc = 'r';
	}else{
		regc = 'e';
	}

	adddef("UCC_WORD_SIZE", b64 ? "8" : "4");

#define ADD_REG(C, c) \
	sprintf(buf, "%c"c, regc); \
	adddef("UCC_REG_" C, buf)

	ADD_REG("A", "ax");
	ADD_REG("B", "bx");
	ADD_REG("C", "cx");
	ADD_REG("D", "dx");

	ADD_REG("SI", "si");
	ADD_REG("DI", "di");

	ADD_REG("SP", "sp");
	ADD_REG("BP", "bp");

	adddef("__FILE__", "");
	adddef("__LINE__", "");
	adddef("__COUNTER__", "");
}

enum proc_ret preprocess(struct pp *p, int verbose)
{
	struct def *d;
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

		fputs("defs:\n", stderr);
		for(d = defs; d; d = d->next)
			fprintf(stderr, "%s = %s\n", d->name, d->val);

		for(i = 0; dirs[i]; i++)
			fprintf(stderr, "include dir \"%s\"\n", dirs[i]);
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
