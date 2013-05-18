#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

#include "expr.h"
#include "expr_tok.h"
#include "str.h"

#include "../util/alloc.h"

enum tok tok_cur;
char *tok_pos;
expr_n tok_cur_num;

void tok_next()
{
	tok_pos = str_spc_skip(tok_pos);

	if(isalpha(*tok_pos) || *tok_pos == '_'){
		/* don't save the ident - it's zero */
		tok_pos = word_end(tok_pos);
		tok_cur = tok_ident;
		return;
	}

	if(isdigit(*tok_pos)){
		char *ep;
		tok_cur_num = strtol(tok_pos, &ep, 0);
		tok_pos = ep;
		tok_cur = tok_num;
		return;
	}

	tok_cur = *tok_pos++;

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
