#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>

#include "expr.h"
#include "expr_tok.h"
#include "str.h"

#include "preproc.h"
#include "main.h" /* CPP_DIE */

#include "../util/alloc.h"
#include "../util/str.h"
#include "../util/util.h"
#include "../util/escape.h"

static char *tok_pos;

enum tok tok_cur;
expr_n tok_cur_num;

void tok_begin(char *begin)
{
	tok_pos = begin;
}

const char *tok_last(void)
{
	return tok_pos - 1;
}

void tok_next()
{
	if(!*tok_pos){
		tok_cur = tok_eof;
		return;
	}

	tok_pos = str_spc_skip(tok_pos);

	if(isdigit(*tok_pos)){
		char *ep;
		tok_cur_num = strtol(tok_pos, &ep, 0);
		do switch(*ep){
			case 'L':
			case 'U':
				ep++;
				break;
			default:
				goto end_ty;
		}while(1);
end_ty:
		tok_pos = ep;
		tok_cur = tok_num;
		return;
	}

	if(*tok_pos == '\''
	|| (tok_pos[0] == 'L' && tok_pos[1] == '\''))
	{
		/* char literal */
		int mchar;
		const int wide = (*tok_pos == 'L');
		int warn;
		int err;

		if(wide)
			tok_pos++;

		tok_pos++;
		warn = err = 0;
		tok_cur_num = escape_char(
				tok_pos, NULL, &tok_pos, wide,
				&mchar, &warn, &err);

		if(err){
			assert(err == EILSEQ);
			CPP_DIE("incomplete escape sequence in literal");
		}

		if(!tok_pos)
			CPP_DIE("missing terminating single quote (\"%s\")", tok_pos);

		if(mchar)
			CPP_WARN(WMULTICHAR, "multi-char constant");

		switch(warn){
			case 0:
				break;
			case ERANGE:
				CPP_WARN(WESCAPE, "character escape out of range");
				break;
			case EINVAL:
				CPP_WARN(WESCAPE, "invalid escape character");
				break;
		}

		tok_cur = tok_num;

		return;
	}

	if(isalpha(*tok_pos) || *tok_pos == '_'){
		/* don't save the ident - it's zero */
		tok_pos = word_end(tok_pos);
		tok_cur = tok_ident;
		return;
	}

	tok_cur = *tok_pos++;
	if(!tok_cur){
		tok_pos--;
		return;
	}

	/* check for double-char ops, e.g. <<, >=, etc */
	switch(*tok_pos){
#define DOUBLE(ch, t)        \
		case ch:                 \
			if(tok_cur == ch)      \
				tok_cur = tok_ ## t, \
				tok_pos++;           \
			break

			DOUBLE('|', orsc);
			DOUBLE('&', andsc);
			DOUBLE('<', shiftl);
			DOUBLE('>', shiftr);
#undef DOUBLE

		case '=':
			switch(tok_pos[-1]){
#define EQ(ch, op) case ch: tok_cur = tok_ ## op; tok_pos++; break
				EQ('=', eq);
				EQ('!', ne);
				EQ('<', le);
				EQ('>', ge);
#undef EQ
			}
	}
}
