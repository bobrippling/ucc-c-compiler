#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>

#include "../util/util.h"
#include "../util/dynarray.h"
#include "../util/alloc.h"
#include "../util/str.h"
#include "../util/path.h"

#include "directive.h"
#include "tokenise.h"
#include "main.h"
#include "macro.h"
#include "preproc.h"
#include "include.h"
#include "str.h"
#include "eval.h"
#include "expr.h"
#include "deps.h"

#define SINGLE_TOKEN(...) \
	tokens = tokens_skip_whitespace(tokens);                        \
	if(tokens_count_skip_spc(tokens) != 1 || tokens[0]->tok != TOKEN_WORD) \
		CPP_DIE(__VA_ARGS__)

#define NO_TOKEN(err) \
	if(tokens_count_skip_spc(tokens) > 0) \
		CPP_WARN(WTRAILING, err " directive with extra tokens")

#define NOOP_RET() if(parse_should_noop()) return

enum if_elif_state
{
	NO_TRUTH,
	GOT_TRUTH,
	LAST_ELSE
};

#define N_IFSTACK 64
static struct
{
	where loc;
	enum if_elif_state if_chosen;
	char noop;
} if_stack[N_IFSTACK];

static int if_idx = 0;
static int noop = 0;
static enum if_elif_state if_elif_chosen = NO_TRUTH;


int parse_should_noop(void)
{
	int i;

	/*
	 * all must be zero to noop
	 * i.e. every noop for each ifdef of the current position
	 * should be 0, meaning no noops
	 */
	if(noop)
		return 1;

	/* TODO: optimise with memcmp? */
	for(i = 0; i < if_idx; i++)
		if(if_stack[i].noop)
			return 1;

	return 0;
}

static void handle_define(token **tokens)
{
	char *name;

	if(tokens[0]->tok != TOKEN_WORD)
		CPP_DIE("word expected");

	NOOP_RET();

	name = tokens[0]->w;

	if(tokens[1]
	&& tokens[1]->tok == TOKEN_OPEN_PAREN
	&& !tokens[1]->had_whitespace)
	{
		/* function macro */
		int i, variadic;
		char **args;
		char *val;

		variadic = 0;
		args = NULL;

		for(i = 2; tokens[i]; i++){
			switch(tokens[i]->tok){
				case TOKEN_CLOSE_PAREN:
					i++;
					goto for_fin;

				case TOKEN_ELIPSIS:
					variadic = 1;
					i++;
					if(tokens[i]->tok != TOKEN_CLOSE_PAREN)
						CPP_DIE("expected: close paren");
					i++;
					goto for_fin;

				case TOKEN_WORD:
					dynarray_add(&args, ustrdup(token_str(tokens[i])));

					i++;
					switch(tokens[i]->tok){
						case TOKEN_COMMA:
							continue;
						case TOKEN_CLOSE_PAREN:
							i++;
							goto for_fin;
						default:
							CPP_DIE("expected: comma or close paren");
					}

				default:
					CPP_DIE("unexpected token %s", token_str(tokens[i]));
			}
		}
for_fin:
		if(!tokens[i]){
			val = ustrdup("");
		}else{
			/* prevent leading whitespace */
			tokens[i]->had_whitespace = 0;

			val = tokens_join(tokens + i);
		}

		macro_add_func(name, val, args, variadic, preproc_in_include());

		free(val);

	}else{
		char *val;

		if(tokens[1]){
			if(!tokens[1]->had_whitespace)
				CPP_WARN(WWHITESPACE, "no whitespace after macro name (%s)", name);
			/* prevent initial whitespace */
			tokens[1]->had_whitespace = 0;
		}

		val = tokens_join(tokens + 1);

		macro_add(name, val, preproc_in_include());

		free(val);
	}
}

static void handle_undef(token **tokens)
{
	char *nam;

	SINGLE_TOKEN("invalid undef macro");

	NOOP_RET();

	nam = tokens[0]->w;
	if(!macro_remove(nam))
		CPP_WARN(WUNDEF_NDEF, "macro \"%s\" not defined", nam);
}

static void handle_error_warning(token **tokens, int err)
{
	where w;
	char *s;

	if(!err && (wmode & WHASHWARNING) == 0)
		return;

	s = tokens_join(tokens);

	warn_colour(1, err);

	/* we're already on the next line */
	where_current(&w);
	w.fname = file_stack[file_stack_idx].fname;
	w.line--;

#ifdef __clang__
	/* this works around clang's buggy __attribute__((noreturn))
	 * merging in a ?: expression
	 */
	void (*fn)(const struct where *, const char *, ...) = err ? die_at : warn_at;
	fn(&w, "#%s:%s", err ? "error" : "warning", s);
#else
	(err ? die_at : warn_at)(&w,
			"#%s:%s", err ? "error" : "warning", s);
#endif

	warn_colour(0, err);

	free(s);

	if(err)
		exit(1);
}

static void handle_warning(token **tokens)
{
	NOOP_RET();
	handle_error_warning(tokens, 0);
}

static void handle_error(token **tokens)
{
	NOOP_RET();
	handle_error_warning(tokens, 1);
}

char *include_parse(
		char *include_arg_anchor, int *const is_lib,
		int may_expand_macros)
{
	char *include_arg = str_spc_skip(include_arg_anchor);
	char *fin;

	*is_lib = 0;

	switch(*include_arg){
		case '<':
			*is_lib = 1;
			include_arg++;
			fin = str_quotefin2(include_arg, '>');
			break;

		case '"':
			include_arg++;
			fin = str_quotefin(include_arg);
			break;

		default:
			if(may_expand_macros){
				char *expanded = eval_expand_macros(ustrdup(include_arg));
				str_trim(expanded);
				return include_parse(expanded, is_lib, 0);
			}else{
				CPP_DIE("bad include start: %c", *include_arg);
			}
	}

	if(!fin)
		CPP_DIE("unterminated #include directive");

	*fin = '\0';

	return ustrdup(include_arg);
}

static void handle_include(char *include_arg)
{
	int is_angle, is_sysh;

	const char *curdir;
	char *fname, *final_path;

	FILE *f = NULL;

	NOOP_RET();

	fname = include_parse(include_arg, &is_angle, 1);
	curdir = cd_stack[dynarray_count(cd_stack) - 1];

	f = include_fopen(curdir, fname, is_angle, &final_path, &is_sysh);
	if(!f){
		if(missing_header_error){
			CPP_DIE("can't find include file %c%s%c",
					"\"<"[is_angle], fname, "\">"[is_angle]);
		}else{
			/* okay - ignored except for dep */
			deps_add(fname);
			goto out;
		}
		/* unreachable */
	}

	/* successfully opened */
	canonicalise_path(final_path);

	if(!is_angle)
		deps_add(final_path);

	preproc_push(f, final_path, is_sysh);
	dirname_push(udirname(final_path));

out:
	free(final_path);

	free(fname);
}

static void if_push(int is_true)
{
	if_stack[if_idx].noop      = noop;
	if_stack[if_idx].if_chosen = if_elif_chosen;
	where_current(&if_stack[if_idx].loc);
	if_stack[if_idx].loc.line--;

	if_idx++;

	if(if_idx == N_IFSTACK)
		die("ifdef stack exceeded");

	noop = !is_true;
	/* we've found a true #if/#ifdef/#ifndef ? */
	if_elif_chosen = is_true ? GOT_TRUTH : NO_TRUTH;
}

static void if_pop(void)
{
	if(if_idx == 0)
		ICE("if_idx == 0 on if_pop()");

	if_idx--;

	noop           = if_stack[if_idx].noop;
	if_elif_chosen = if_stack[if_idx].if_chosen;
}

static void handle_somedef(token **tokens, int rev)
{
	tokens = tokens_skip_whitespace(tokens);
	SINGLE_TOKEN("too many arguments to ifdef macro");

	if(noop)
		if_push(0);
	else
		if_push(rev ^ !!macro_find(tokens[0]->w));
}

static void handle_ifdef(token **tokens)
{
	handle_somedef(tokens, 0);
}

static void handle_ifndef(token **tokens)
{
	handle_somedef(tokens, 1);
}

static int /*bool*/ if_eval(token **tokens, const char *type)
{
	char *w;
	expr *e;

	if(!tokens)
		CPP_DIE("#%s needs arguments", type);

	w = tokens_join(tokens);

	for(;;){
		/* first we need to filter out defined() */
		w = eval_expand_defined(w);
		w = eval_expand_has_include(w);

		/* then macros */
		w = eval_expand_macros(w);

		/* then maybe refilter out defined() again */
		if(word_find(w, DEFINED_STR))
			continue;
		break;
	}

	/* then parse */
	e = expr_parse(w);

	/* and eval (this also frees e) */
	{
		int had_ident = 0;
		const int r = !!expr_eval(e, &had_ident, 0);

		if(had_ident)
			CPP_WARN(WUNDEF_IN_IF,
					"undefined identifier in: \"#%s%s\"", type, w);

		free(w);

		return r;
	}
}

static void handle_if(token **tokens)
{
	int is_true;
	if(noop)
		is_true = 0;
	else
		is_true = !!if_eval(tokens, "if");

	if_push(is_true);
}

static void got_else(const char *type)
{
	if(if_idx == 0)
		CPP_DIE("%s unexpected", type);
}

static void handle_elif(token **tokens)
{
	got_else("elif");

	switch(if_elif_chosen){
		case GOT_TRUTH:
			noop = 1;
			break;
		case NO_TRUTH:
			if_elif_chosen = if_eval(tokens, "elif");
			noop = !if_elif_chosen;
			break;
		case LAST_ELSE:
			CPP_DIE("#else already encountered (now at #elif)");
	}
}

static void handle_else(token **tokens)
{
	NO_TOKEN("else");

	got_else("else");

	switch(if_elif_chosen){
		case GOT_TRUTH:
			noop = 1;
			break;
		case NO_TRUTH:
			noop = 0;
			break;
		case LAST_ELSE:
			CPP_DIE("#else already encountered (now at #else)");
	}
	if_elif_chosen = LAST_ELSE;
}

static void handle_endif(token **tokens)
{
	NO_TOKEN("endif");

	if(if_idx == 0)
		CPP_DIE("endif unexpected");

	if_pop();
}

static void handle_pragma(token **tokens)
{
	/* pass to cc1 */
	char *out = tokens_join(tokens);
	printf("#pragma %s\n", out);
	free(out);
}

static int handle_line_directive(char *line)
{
	int need_line = 0;
	int n;
	char *endp;
	char *output_anchor;

	line = str_spc_skip(line);

	if(!strncmp(line, "line", 4)){
		char *end = word_end(line);
		if(end != line + 4)
			return 0;

		line += 4;
		need_line = 1;
	}
	line = str_spc_skip(line);

	n = strtol(line, &endp, 0);
	if(endp == line){
		if(need_line)
			CPP_DIE("invalid #line directive");
		/* else we've had # something */
		return 0;
	}

	if(n < 0)
		CPP_DIE("#line directive with a negative argument");

	/* parse the filename, if given */
	output_anchor = line;
	line = str_spc_skip(endp);
	*endp = '\0';
	if(*line == '"'){
		char *fname = line + 1;
		char *end = strchr(fname, '"');

		if(!*fname || !end)
			CPP_DIE("#line directive has unterminated double-quote");

		*line = '\0';
		*end = '\0';
		set_current_fname(fname);
	}

	/* don't care about the filename - that's for cc1 */
	preproc_emit_line_info(atoi(output_anchor), current_fname, /* no line-info */0);
	current_line = n - 1;

	return 1;
}

static void directive_sync(void)
{
	if(!no_output)
		putchar('\n'); /* keep line-no.s in sync */
}

void parse_directive(char *line)
{
	token **tokens = NULL;

	/* check for /# *[0-9]+ *( +"...")?/ */
	if(handle_line_directive(line))
		goto fin;

	/* check for include - we handle it specially
	 * because <> need to be handled like quotes */
	if(!parse_should_noop()){
		const char *const inc = "include";
		char *start = str_spc_skip(line);
		char *end = word_end(start);
		char save = *end;
		int is_inc;

		*end = '\0';
		is_inc = !strcmp(start, inc);
		*end = save;

		if(is_inc){
			directive_sync();

			handle_include(start + strlen(inc));
			return;
		}
	}

	tokens = tokenise(line);

	if(!tokens)
		return;

	if(tokens[0]->tok != TOKEN_WORD){
		if(parse_should_noop())
			goto fin;

		CPP_DIE("invalid preproc token");
	}

	directive_sync();

#define HANDLE(s)                \
	if(!strcmp(tokens[0]->w, #s)){ \
		handle_ ## s(tokens + 1);    \
		goto fin;                    \
	}

	HANDLE(ifdef)
	HANDLE(ifndef)
	HANDLE(if)
	HANDLE(elif)
	HANDLE(else)
	HANDLE(endif)

	if(parse_should_noop())
		goto fin; /* checked for flow control, nothing else so noop */

	HANDLE(define)
	HANDLE(undef)

	HANDLE(warning)
	HANDLE(error)

	HANDLE(pragma)

	CPP_DIE("unrecognised preproc command \"%s\"", tokens[0]->w);
fin:
	tokens_free(tokens);
}

void parse_internal_directive(char *line)
{
	no_output = 1;
	parse_directive(line);
	no_output = 0;
}

void parse_end_validate()
{
	if(if_idx){
		char buf[WHERE_BUF_SIZ];

		CPP_DIE("endif expected\n"
				"%s: note: to match this",
				where_str_r(buf, &if_stack[if_idx - 1].loc));
	}
}
