#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include "pp.h"
#include "utils.h"

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

static void die(struct pp *p, const char *fmt, ...)
{
	va_list l;

	if(p)
		fprintf(stderr, "%s:%d: ", p->fname, p->nline);

	va_start(l, fmt);
	vfprintf(stderr, fmt, l);
	va_end(l);
	exit(1);
}

void adddir(const char *d)
{
	if(!dirs){
		dirs = malloc(2 * sizeof(*dirs));
		if(!dirs)
			die(NULL, "malloc: %s\n", strerror(errno));
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
	struct def *d = malloc(sizeof *d);

	n2 = strdup(n);
	v2 = strdup(v);

	if(!n2 || !v2)
		die(NULL, "strdup(): %s\n", strerror(errno));
	else if(!d)
		die(NULL, "malloc %ld bytes\n", sizeof *d);

	strcpy(n2, n);
	strcpy(v2, v);

	d->name = n2;
	d->val  = v2;

	d->next = defs;
	defs = d;

	if(pp_verbose)
		fprintf(stderr, "adddef(\"%s\", \"%s\")\n", n, v);
}

static char *strdup_printf(struct pp *p, const char *fmt, ...)
{
	va_list l;
	char *buf = NULL;
	int len = 8, ret;

	do{
		len *= 2;
		buf = realloc(buf, len);
		if(!buf)
			die(p, "realloc()\n");

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

			new = strdup_printf(p, "%s%s%s", *line, val, post);
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
	char *line, *nl;

	do{
		line = readline(p->in);
		p->nline++;

		if(!line){
			if(feof(p->in))
				/* normal exit here */
				return PROC_EOF;

			die(p, "read(): %s\n", strerror(errno));
		}

		if((nl = strchr(line, '\n')))
			*nl = '\0';

#define MACRO(str) !strncmp(line, "#" str, strlen("#" str))

		if(*line == '#'){
			if(MACRO("include ")){
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
							die(p, "invalid include char %c\n", *base);
					}

					for(path = ++base; *path; path++)
						if(*path == incchar){
							*path = '\0';
							found = 1;
							break;
						}

					if(!found)
						die(p, "no terminating %c for include \"%s\"\n", incchar, line);

					found = 0;
					/* FIXME: switch on incchar */
					for(i = 0; i < ndirs; i++){
						path = strdup_printf(p, "%s/%s", dirs[i], base);

						inc = fopen(path, "r");

						if(inc){
							if(pp_verbose)
								fprintf(stderr, "including %s\n", path);
							found = 1;
							pp2.fname = path;
							break;
						}
						free(path);
					}

					if(!found)
						die(p, "can't find include file \"%s\"\n", base);

					pp2.in  = inc;
					pp2.out = p->out;

					switch(pp(&pp2, 0)){
						case PROC_EOF:
							break;

						case PROC_ELSE:
						case PROC_ENDIF:
						case PROC_ERR:
							die(p, "eof expected from including \"%s\"\n", pp2.fname);
					}

					if(pp_verbose)
						fprintf(stderr, "included %s\n", path);

					fclose(inc);
					free(path); /* pp2.fname */
				}
			}else if(MACRO("define ")){
				if(!skip){
					char *word, *space = line + 8;

					while(isspace(*space))
						space++;

					if(*space){
						word = space;
						while(*space && !isspace(*space))
							space++;

						if(*space)
							*space++ = '\0';
						else
							space = "";
					}

					adddef(word, space);
				}
			}else if(MACRO("ifdef ")){
				struct pp arg;
				int gotdef;
				int ret;

				if(skip)
					gotdef = 0;
				else
					gotdef = !!getdef(line + 7);

				memcpy(&arg, p, sizeof arg);
				arg.out = devnull;

				ret = pp(gotdef ? p : &arg, skip || !gotdef);

				switch(ret){
					case PROC_ELSE:
						ret = pp(gotdef ? &arg : p, skip || gotdef);
						if(ret != PROC_ENDIF)
							die(p, "endif expected\n");
					case PROC_ENDIF:
						break;

					case PROC_EOF:
						die(p, "eof unexpected\n");
				}
			}else if(MACRO("else")){
				free(line);
				return PROC_ELSE;
			}else if(MACRO("endif")){
				free(line);
				return PROC_ENDIF;
			}else{
				die(p, "%s unexpected\n", line+1);
			}
		}else{
			substitutedef(p, &line);
			fprintf(p->out, "%s\n", line);
		}
		free(line);
	}while(1);
}

void def_defs(void)
{
	char buf[8];
	char regc;
	int b64;

	b64 = getarch() == ARCH_64;

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
