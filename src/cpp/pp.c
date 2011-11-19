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

struct def
{
	char *name, *val;
	struct def *next;
};


static struct def *defs = NULL;
static const char **dirs = NULL;
static int    ndirs = 0;
static FILE  *devnull;
static int    pp_verbose = 0;

int current_line = 0, current_chr = 0;
const char *current_fname = NULL;

static void ppdie(struct pp *p, const char *fmt, ...)
{
	va_list l;
	struct where where;

	if(p){
		where.fname = p->fname;
		where.line  = p->nline;
		where.chr   = -1;
	}

	va_start(l, fmt);
	vdie(&where, l, fmt);
}

void adddir(const char *d)
{
	if(!dirs){
		dirs = umalloc(2 * sizeof(*dirs));
		dirs[0] = d;
		dirs[1] = NULL;
		ndirs = 1;
	}else{
		dirs = realloc(dirs, (ndirs + 2) * sizeof(*dirs));
		if(!dirs)
			die(NULL, "realloc: %s\n", strerror(errno));

		dirs[ndirs++] = d;
		dirs[ndirs  ] = NULL;
	}

	if(pp_verbose)
		fprintf(stderr, "adddir(\"%s\")\n", d);
}

void adddef(const char *n, const char *v)
{
	/* FIXME: check for substring v in n */
	char *n2, *v2;
	struct def *d = umalloc(sizeof *d);

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

static void substitutedef(struct pp *p, char **line)
{
	struct def *d;
	char *pos;

	for(d = defs; d; d = d->next){
		while((pos = strstr(*line, d->name))){
			char *const post = pos + strlen(d->name);
			char *new;
			const char *val;

			*pos = post[-1] = '\0';

			if(!strcmp(d->name, "__LINE__")){
				static char linebuf[16];
				snprintf(linebuf, sizeof linebuf, "%d", p->nline);
				val = linebuf;
			}else if(!strcmp(d->name, "__FILE__")){
				val = p->fname; /* FIXME: quote */
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

static int pp(struct pp *p, int skip)
{
#define RET(x) do{ ret = x; goto fin; }while(0)
	int curwdfd;
	int ret;
	char *line, *nl;
	char *wd;

	/* save for "cd -" */
	curwdfd = open(".", O_RDONLY);
	if(curwdfd == -1)
		ppdie(p, "open(\".\"): %s", strerror(errno));

	/* make sure everything is relative to the file */
	wd = udirname(p->fname);
	if(chdir(wd))
		ppdie(p, "chdir(\"%s\"): %s", wd, strerror(errno));


	do{
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
						case '<':
							incchar = *base;
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

					/* FIXME: switch on incchar */

					if(incchar == '<'){
						for(i = 0; i < ndirs; i++){
							path = strdup_printf("%s/%s", dirs[i], base);

							inc = fopen(path, "r");

							if(inc){
								found = 1;
								break;
							}

							free(path);
						}

						if(!found)
							ppdie(p, "can't find include file \"%s\"", base);

					}else{
						path = ustrdup(base);
						inc = fopen(path, "r");
						if(!inc){
							char pwd[1024];
							getcwd(pwd, sizeof pwd);
							ppdie(p, "can't open \"%s\": %s (pwd %s)", path, strerror(errno), pwd);
						}
					}

					if(pp_verbose)
						fprintf(stderr, "including %s\n", path);
					pp2.fname = path;

					pp2.in  = inc;
					pp2.out = p->out;

					switch(pp(&pp2, 0)){
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

				if(!skip)
					adddef(argv[1], argv[2] ? argv[2] : "");

			}else if(!strcmp(argv[0], "ifdef")){
				struct pp arg;
				int gotdef;
				int ret;

				if(argc != 2)
					ppdie(p, "ifdef takes one argument");

				if(skip)
					gotdef = 0;
				else
					gotdef = !!getdef(argv[1]);

				memcpy(&arg, p, sizeof arg);
				arg.out = devnull;

				ret = pp(gotdef ? p : &arg, skip || !gotdef);

				switch(ret){
					case PROC_ELSE:
						ret = pp(gotdef ? &arg : p, skip || gotdef);
						if(ret != PROC_ENDIF)
							ppdie(p, "endif expected");
					case PROC_ENDIF:
						break;

					case PROC_EOF:
						ppdie(p, "eof unexpected");
				}
			}else if(!strcmp(argv[0], "else")){
				if(argc != 1)
					ppdie(p, "invalid #else");

				free(line);
				RET(PROC_ELSE);
			}else if(!strcmp(argv[0], "endif")){
				if(argc != 1)
					ppdie(p, "invalid #endif");

				free(line);
				RET(PROC_ENDIF);
			}else{
				ppdie(p, "\"%s\" unexpected", line);
			}
		}else{
			substitutedef(p, &line);
			fprintf(p->out, "%s\n", line);
		}
		free(line);
	}while(1);
#undef RET
fin:
	if(fchdir(curwdfd) == -1)
		ppdie(p, "chdir(\"-\"): %s", strerror(errno));
	close(curwdfd);
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

		for(i = 0; i < ndirs; i++)
			fprintf(stderr, "include dir \"%s\"\n", dirs[i]);
	}

	ret = pp(p, 0);
	freedefs();

	fclose(devnull);
	devnull = NULL;

	return ret != PROC_EOF;
}
