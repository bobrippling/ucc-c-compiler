#include <stdio.h>
#include <string.h>

#include "../util/io.h"
#include "../util/str.h"
#include "../util/alloc.h"
#include "../util/dynarray.h"

#include "spec.h"
#include "ucc_ext.h"
#include "str.h"

static int var_lookup_str(
		const char *varname,
		const char **const varvalue,
		const struct specvars *vars)
{
	if(!strcmp(varname, "out")){
		*varvalue = vars->output;
		return 1;
	}
	if(!strcmp(varname, "bindir")){
		*varvalue = ucc_where();
		return 1;
	}
	return 0;
}

static int var_lookup_bool(
		const char *varname,
		int *const varvalue,
		const struct specvars *vars)
{
	if(!strcmp(varname, "startfiles")){
		*varvalue = vars->startfiles;
		return 1;
	}
	if(!strcmp(varname, "debug")){
		*varvalue = vars->debug;
		return 1;
	}
	if(!strcmp(varname, "stdlib")){
		*varvalue = vars->stdlib;
		return 1;
	}
	if(!strcmp(varname, "stdinc")){
		*varvalue = vars->stdinc;
		return 1;
	}
	if(!strcmp(varname, "shared")){
		*varvalue = vars->shared;
		return 1;
	}
	if(!strcmp(varname, "static")){
		*varvalue = vars->static_;
		return 1;
	}
	return 0;
}

static char *nested_close_curly(char *in)
{
	size_t nest = 0;
	for(; *in; in++){
		switch(*in){
			case '{':
				nest++;
				break;
			case '}':
				if(nest == 0)
					return in;
				nest--;
				break;
		}
	}
	return NULL;
}

static char *spec_interpolate(char *line, const struct specvars *vars)
{
	size_t anchor = 0;
	char *percent;

	while((percent = strstr(line + anchor, "%{"))){
		char *close, *colon;
		char *varname = percent + 2;
		const char *replacewith = NULL;

		anchor = (percent - line) + 1;

		close = nested_close_curly(varname);
		if(!close)
			continue;

		*close = '\0';

		colon = strchr(percent, ':');
		if(colon && colon < close){
			/* %{bool:str} */
			int varvalue;

			*colon = '\0';

			if(var_lookup_bool(varname, &varvalue, vars)){
				if(varvalue){
					replacewith = colon + 1;
				}else{
					replacewith = "";
				}
			}else{
				fprintf(stderr, "no such spec boolean \"%s\"\n", varname);
				*colon = ':';
			}

		}else{
			const char *varvalue;

			if(var_lookup_str(varname, &varvalue, vars)){
				replacewith = varvalue;
			}else{
				/* nothing to do */
				fprintf(stderr, "no such spec string \"%s\"\n", varname);
			}
		}

		if(replacewith){
			char *new;
			*percent = '\0';
			new = ustrprintf("%s%s%s", line, replacewith, close + 1);
			free(line);
			line = new;
		}else{
			*close = '}';
		}
	}

	return line;
}

static char *spec_ignore_space(char *line)
{
	char *p = str_spc_skip(line);

	if(p != line)
		memmove(line, p, strlen(p) + 1);

	return line;
}

void spec_parse(
		struct specopts *opts,
		const struct specvars *vars,
		FILE *file,
		struct specerr *err)
{
	char *current_block = NULL;
	char *line = NULL;
	unsigned current_block_line = 0;

	memset(err, 0, sizeof(*err));
	memset(opts, 0, sizeof(*opts));

	while(err->errline++, free(line), line = fline(file, NULL)){
		char *comment = strchr(line, '#');
		char *p;

		if(comment)
			*comment = '\0';

		p = strchr(line, '\n');
		if(p)
			*p = '\0';

		p = str_spc_skip(line);
		if(!*p)
			continue;

		if(p == line
		&& (p = strchr(p, ':'))
		&& p[1] == '\0')
		{
			*p = '\0';
			free(current_block);
			current_block = line;
			line = NULL;
			current_block_line = err->errline;
		}
		else
		{
			if(!current_block){
				err->errstr = "directive outside block";
				break;
			}

			p = NULL;
			line = spec_interpolate(line, vars);
			line = spec_ignore_space(line);

			if(!strcmp(current_block, "initflags")){
				dynarray_add_tmparray(&opts->initflags, strsplit(line, " \t"));

			}else if(!strcmp(current_block, "as")){
				free(opts->as), opts->as = line;
			}else if(!strcmp(current_block, "asflags")){
				dynarray_add_tmparray(&opts->asflags, strsplit(line, " \t"));

			}else if(!strcmp(current_block, "ld")){
				free(opts->ld), opts->ld = line;
			}else if(!strcmp(current_block, "ldflags_pre_user")){
				dynarray_add_tmparray(&opts->ldflags_pre_user, strsplit(line, " \t"));
			}else if(!strcmp(current_block, "ldflags_post_user")){
				dynarray_add_tmparray(&opts->ldflags_post_user, strsplit(line, " \t"));

			}else if(!strcmp(current_block, "post-link")){
				free(opts->post_link), opts->post_link = line;

			}else{
				err->errstr = "unknown block";
				err->errline = current_block_line;
				break;
			}

			line = NULL;
		}
	}

	free(line), line = NULL;
}

static void dump_ar(const char *name, char **array)
{
	fprintf(stderr, "  %s:\n", name);
	for(char **p = array; p && *p; p++)
		fprintf(stderr, "    %s\n", *p);
}

void spec_dump(struct specopts *opts, FILE *f)
{
	fprintf(f, "spec:\n");

	dump_ar("initflags", opts->initflags);

	fprintf(f, "  as: %s\n", opts->as);
	dump_ar("asflags", opts->asflags);

	fprintf(f, "  ld: %s\n", opts->ld);
	dump_ar("ldflags_pre_user", opts->ldflags_pre_user);
	dump_ar("ldflags_post_user", opts->ldflags_post_user);

	fprintf(f, "  post_link: %s\n", opts->post_link);
}
