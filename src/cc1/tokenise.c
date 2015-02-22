#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <stdarg.h>
#include <limits.h>
#include <assert.h>

#include "../util/util.h"
#include "tokenise.h"
#include "../util/alloc.h"
#include "../util/str.h"
#include "../util/escape.h"
#include "str.h"
#include "cc1.h"
#include "cc1_where.h"
#include "btype.h"

#ifndef CHAR_BIT
#  define CHAR_BIT 8
#endif

#define KEYWORD(mode, x) { #x, token_ ## x, mode }

/* __ keywords are always enabled */
#define KEYWORD__(x, t)       \
	{ "__" #x,      t, KW_ALL }, \
	{ "__" #x "__", t, KW_ALL }

#define KEYWORD__ALL(mode, x) \
	KEYWORD(mode, x),           \
	KEYWORD__(x, token_ ## x)

static const struct keyword
{
	const char *str;
	enum token tok;
	enum keyword_mode mode;

} keywords[] = {
	KEYWORD(KW_ALL, if),
	KEYWORD(KW_ALL, else),

	KEYWORD(KW_ALL, switch),
	KEYWORD(KW_ALL, case),
	KEYWORD(KW_ALL, default),

	KEYWORD(KW_ALL, break),
	KEYWORD(KW_ALL, return),
	KEYWORD(KW_ALL, continue),

	KEYWORD(KW_ALL, goto),

	KEYWORD(KW_ALL, do),
	KEYWORD(KW_ALL, while),
	KEYWORD(KW_ALL, for),

	KEYWORD(KW_ALL, void),
	KEYWORD(KW_ALL, char),
	KEYWORD(KW_ALL, int),
	KEYWORD(KW_ALL, short),
	KEYWORD(KW_ALL, long),
	KEYWORD(KW_ALL, float),
	KEYWORD(KW_ALL, double),

	KEYWORD(KW_ALL, auto),
	KEYWORD(KW_ALL, static),
	KEYWORD(KW_ALL, extern),
	KEYWORD(KW_ALL, register),

	KEYWORD__ALL(KW_EXT, asm),
	KEYWORD__ALL(KW_EXT | KW_C99, inline),

	KEYWORD__ALL(KW_ALL, const),
	KEYWORD__ALL(KW_ALL, volatile),
	KEYWORD__ALL(KW_C99, restrict),

	KEYWORD__ALL(KW_ALL, signed),
	KEYWORD__ALL(KW_ALL, unsigned),

	KEYWORD(KW_ALL, typedef),
	KEYWORD(KW_ALL, struct),
	KEYWORD(KW_ALL, union),
	KEYWORD(KW_ALL, enum),

	KEYWORD(KW_ALL, _Bool), /* reserved namespace - fine for C89 */
	KEYWORD(KW_ALL, _Noreturn),

	KEYWORD(KW_ALL, _Alignof),
	KEYWORD__(alignof, token__Alignof),
	KEYWORD(KW_ALL, _Alignas),
	KEYWORD__(alignas, token__Alignas),

	KEYWORD(KW_ALL, sizeof),
	KEYWORD__ALL(KW_EXT, typeof),
	KEYWORD(KW_ALL, _Generic),
	KEYWORD(KW_ALL, _Static_assert),

	KEYWORD(KW_ALL, __builtin_va_list),
	KEYWORD(KW_ALL, __auto_type),

	KEYWORD(KW_ALL, __extension__),
	KEYWORD__(attribute, token_attribute),

	{ NULL, 0, 0 },
};

static tokenise_line_f *in_func;
int buffereof = 0;
static int parse_finished = 0;
static enum keyword_mode keyword_mode = KW_ALL;

#define FNAME_STACK_N 32
static struct fnam_stack
{
	char *fnam;
	int    lno;
} current_fname_stack[FNAME_STACK_N];

static int current_fname_stack_cnt;
char *current_fname;
static int current_fname_used;

static char *buffer, *bufferpos;
static int ungetch = EOF;

static int in_comment;

static struct line_list
{
	char *line;
	struct line_list *next;
} *store_lines, **store_line_last = &store_lines;

/* -- */
enum token curtok, curtok_uneat;
int parse_had_error;

numeric currentval = { { 0 } }; /* an integer literal */

char *currentspelling = NULL; /* e.g. name of a variable */

char *currentstring   = NULL; /* a string literal */
size_t currentstringlen = 0;
int   currentstringwide = 0;
where currentstringwhere;

/* where the parser is, and where the last parsed token was */
static struct loc loc_now;
struct loc loc_tok;

char *current_line_str = NULL;
int current_line_str_used = 0;

#define SET_CURRENT(ty, new) do{\
	if(!current_ ## ty ## _used)  \
		free(current_ ## ty);       \
	current_## ty = new;          \
	current_## ty ##_used = 0; }while(0)

#define SET_CURRENT_LINE_STR(new) SET_CURRENT(line_str, new)


struct where *where_cc1_current(struct where *w)
{
	static struct where here;

	if(!w) w = &here;

	/* XXX: current_chr positions at the end of the current token */
	where_current(w);

	current_fname_used = 1;
	current_line_str_used = 1;

	return w;
}

void where_cc1_adj_identifier(where *w, const char *sp)
{
	w->len = strlen(sp);
}


static void push_fname(char *fn, int lno)
{
	current_fname = fn;
	if(current_fname_stack_cnt < FNAME_STACK_N){
		struct fnam_stack *p = &current_fname_stack[current_fname_stack_cnt++];
		p->fnam = ustrdup(fn);
		p->lno = lno;
	}
}

static void pop_fname(void)
{
	if(current_fname_stack_cnt > 0){
		struct fnam_stack *p = &current_fname_stack[--current_fname_stack_cnt];
		free(p->fnam);
	}
}

static void handle_line_file_directive(char *fnam, int lno)
{
	/*
# 1 "inc.c"
# 5 "yo.h"  // include "yo.h"
            // if we get an error here,
            // we want to know we're included from inc.c:1
# 2 "inc.c" // include end - the line no. doesn't have to be prev+1
	 */

	/* logic for knowing when to pop and when to push */
	int i;

	if(!cc1_first_fname)
		cc1_first_fname = ustrdup(fnam);

	for(i = current_fname_stack_cnt - 1; i >= 0; i--){
		struct fnam_stack *stk = &current_fname_stack[i];

		if(!strcmp(fnam, stk->fnam)){
			/* found another "inc.c" */
			/* pop `n` stack entries, then push our new one */
			while(current_fname_stack_cnt > i)
				pop_fname();
			break;
		}
	}

	push_fname(fnam, lno);
}

static void parse_line_directive(char *l)
{
	int lno;
	char *ep;

	l = str_spc_skip(l + 1);
	if(!strncmp(l, "line", 4))
		l += 4;

	lno = strtol(l, &ep, 0);
	if(ep == l)
		die("couldn't parse number for #line directive (%s)", ep);

	if(lno < 0)
		die("negative #line directive argument");

	loc_now.line = lno - 1; /* inc'd below */

	ep = str_spc_skip(ep);

	switch(*ep){
		case '"':
			{
				char *p = str_quotefin(++ep);
				if(!p)
					die("no terminating quote to #line directive (%s)", l);
				handle_line_file_directive(ustrdup2(ep, p), lno);
				/*l = str_spc_skip(p + 1);
					if(*l)
					die("characters after #line?");
					- gcc puts characters after the string */
				break;
			}
		case '\0':
			break;

		default:
			die("expected '\"' or nothing after #line directive (%s)", ep);
	}
}

void include_bt(FILE *f)
{
	int i;
	for(i = 0; i < current_fname_stack_cnt - 1; i++){
		struct fnam_stack *stk = &current_fname_stack[i];

		fprintf(f, "%s:%d: included from here\n",
				stk->fnam, stk->lno);
	}
}

static void add_store_line(char *l)
{
	struct line_list *new = umalloc(sizeof *new);
	new->line = l;

	*store_line_last = new;
	store_line_last = &new->next;
}

static ucc_wur char *tokenise_read_line()
{
	char *l;

	if(buffereof)
		return NULL;

	l = in_func();
	if(!l){
		buffereof = 1;
	}else{
		/* check for preprocessor line info */
		/* but first - add to store_lines */
		if(fopt_mode & FOPT_SHOW_LINE)
			add_store_line(l);

		/* format is # line? [0-9] "filename" ([0-9])* */
		if(!in_comment && *l == '#'){
			parse_line_directive(l);
			return tokenise_read_line();
		}

		loc_now.chr = 0;
		loc_now.line++;
		loc_tok = loc_now;

		if(current_fname_stack_cnt > 0)
			current_fname_stack[current_fname_stack_cnt - 1].lno = loc_now.line;
	}

	return l;
}

static void tokenise_next_line()
{
	char *new = tokenise_read_line();

	if(new)
		SET_CURRENT_LINE_STR(ustrdup(new));

	if(buffer){
		if((fopt_mode & FOPT_SHOW_LINE) == 0)
			free(buffer);
	}

	bufferpos = buffer = new;
}

static void update_bufferpos(char *new)
{
	loc_now.chr += new - bufferpos;
	bufferpos = new;
}

void tokenise_set_input(tokenise_line_f *func, const char *nam)
{
	char *nam_dup = ustrdup(nam);
	in_func = func;

	if(fopt_mode & FOPT_TRACK_INITIAL_FNAM)
		push_fname(nam_dup, 1);
	else
		current_fname = nam_dup;

	SET_CURRENT_LINE_STR(NULL);

	loc_now.line = buffereof = parse_finished = 0;
	nexttoken();
}

void tokenise_set_mode(enum keyword_mode m)
{
	keyword_mode = m | KW_ALL;
}

char *token_current_spel()
{
	char *ret = currentspelling;
	currentspelling = NULL;
	return ret;
}

char *token_current_spel_peek(void)
{
	return currentspelling;
}

int tok_at_label(void)
{
	/* [a-z]+:
	 * need to cater for newlines
	 */
	char *p = bufferpos;

	if(curtok != token_identifier)
		return 0;

	/* if we're on a #line, ignore */
	if(*bufferpos == '#'){
		parse_line_directive(bufferpos);

	}else for(; *p; p++){
		if(*p == ':'){

			return 1;

		}else if(!isspace(*p)){
			return 0;
		}
	}

	/* read the next line in */
	{
		char *const new = tokenise_read_line();

		if(new){
			size_t const newlen = strlen(new);
			size_t const poff = p - buffer;
			size_t const len = poff + newlen;

			buffer = urealloc1(buffer, len + 1);
			p = buffer + poff;
			memcpy(p, new, newlen + 1);
			bufferpos = p;
			return tok_at_label();
		}
		return 0;
	}
}

static int rawnextchar()
{
	if(buffereof)
		return EOF;

	while(!bufferpos || !*bufferpos){
		tokenise_next_line();
		if(buffereof)
			return EOF;
	}

	loc_now.chr++;

	return *bufferpos++;
}

static int nextchar()
{
	int c;
	do
		c = rawnextchar();
	while(isspace(c) || c == '\f'); /* C allows ^L aka '\f' anywhere in the code */
	return c;
}

static int peeknextchar()
{
	/* doesn't ignore isspace() */
	if(!bufferpos)
		tokenise_next_line();

	if(buffereof)
		return EOF;

	return *bufferpos;
}

static void read_integer(const enum base mode)
{
	char *end;
	int of; /*verflow*/

	switch(mode){
		case BIN: currentval.suffix = VAL_BIN; break;
		case HEX: currentval.suffix = VAL_HEX; break;
		case OCT: currentval.suffix = VAL_OCTAL; break;
		case DEC: currentval.suffix = 0; break;
	}

	currentval.val.i = char_seq_to_ullong(bufferpos, &end, mode, 0, &of);

	if(of){
		/* force unsigned long long ULLONG_MAX */
		currentval.val.i = NUMERIC_T_MAX;
		currentval.suffix = VAL_LLONG | VAL_UNSIGNED;
	}

	if(end == bufferpos)
		die_at(NULL, "%s-number expected (got '%c')",
				base_to_str(mode), peeknextchar());

	update_bufferpos(end);
}

static void read_suffix_float_exp(void)
{
	numeric mantissa;
	int powmul;
	int just_read = nextchar();

	assert(just_read == 'e');

	if(!(currentval.suffix & VAL_FLOATING)){
		currentval.suffix = VAL_DOUBLE; /* 1e2 is double by default */
		currentval.val.f = currentval.val.i;
	}
	mantissa = currentval;

	powmul = (peeknextchar() == '-' ? -1 : 1);
	if(powmul == -1)
		nextchar();

	if(!isdigit(peeknextchar())){
		warn_at_print_error(NULL, "no digits in exponent");
		parse_had_error = 1;
		return;
	}
	read_integer(DEC); /* can't have fractional powers */

	mantissa.val.f *= pow(10, powmul * (sintegral_t)currentval.val.i);

	currentval = mantissa;
}

static void read_suffix_float(void)
{
	if(tolower(peeknextchar()) == 'e'){
		read_suffix_float_exp();
	}

	if(tolower(peeknextchar()) == 'f'){
		currentval.suffix = VAL_FLOAT;
		nextchar();
	}else if(tolower(peeknextchar()) == 'l'){
		currentval.suffix = VAL_LDOUBLE;
		nextchar();
	}else{
		currentval.suffix = VAL_DOUBLE;
	}

	currentval.suffix &= VAL_FLOATING;
}

static void read_suffix_int(void)
{
	/* accept either 'U' 'L' or 'LL' as atomic parts (i.e. not LUL) */
	/* fine using nextchar() since we peeknextchar() first */
	enum numeric_suffix suff = 0;
	char c;

	for(;;) switch((c = peeknextchar())){
		case 'U':
		case 'u':
			if(suff & VAL_UNSIGNED)
				die_at(NULL, "duplicate U suffix");
			suff |= VAL_UNSIGNED;
			nextchar();
			break;
		case 'L':
		case 'l':
			if(suff & (VAL_LLONG | VAL_LONG))
				die_at(NULL, "already have a L/LL suffix");

			nextchar();
			if(peeknextchar() == c){
				C99_LONGLONG();
				suff |= VAL_LLONG;
				nextchar();
			}else{
				suff |= VAL_LONG;
			}
			break;
		default:
			goto out;
	}

out:
	/* don't touch cv.suffix until after
	 * - it may already have ULL from an
	 * overflow in parsing
	 */
	currentval.suffix |= suff;
}

static void read_suffix(void)
{
	/* handle floating XeY */
	if(currentval.suffix & VAL_FLOATING || tolower(peeknextchar())== 'e'){
		read_suffix_float();
	}else{
		read_suffix_int();
	}


	if(isalpha(peeknextchar())){
		warn_at_print_error(NULL,
				"invalid suffix on integer constant (%c)",
				peeknextchar());

		parse_had_error = 1;

		while(isalpha(peeknextchar()))
			nextchar();
	}
}

static enum token curtok_to_xequal(void)
{
#define MAP(x) case x: return x ## _assign
	switch(curtok){
		MAP(token_plus);
		MAP(token_minus);
		MAP(token_multiply);
		MAP(token_divide);
		MAP(token_modulus);
		MAP(token_not);
		MAP(token_bnot);
		MAP(token_and);
		MAP(token_or);
		MAP(token_xor);
		MAP(token_shiftl);
		MAP(token_shiftr);
#undef MAP

		default:
			break;
	}
	return token_unknown;
}

static int curtok_is_xequal()
{
	return curtok_to_xequal() != token_unknown;
}

static void read_string(char **sptr, size_t *plen)
{
	char *const start = bufferpos;
	char *const end = str_quotefin(start);
	size_t size;

	if(!end){
		char *p;
		if((p = strchr(bufferpos, '\n')))
			*p = '\0';
		die_at(NULL, "Couldn't find terminating quote to \"%s\"", bufferpos);
	}

	size = end - start + 1;

	*sptr = umalloc(size);
	*plen = size;

	strncpy(*sptr, start, size);
	(*sptr)[size-1] = '\0';

	escape_string(*sptr, plen);

	update_bufferpos(bufferpos + size);
}

static void ungetchar(char ch)
{
	if(ungetch != EOF)
		ICE("ungetch");

	ungetch = ch;
}

static int getungetchar(void)
{
	const int ch = ungetch;
	ungetch = EOF;
	return ch;
}

static void read_string_multiple(const int is_wide)
{
	/* TODO: read in "hello\\" - parse string char by char, rather than guessing and escaping later */
	char *str;
	size_t len;

	where_cc1_current(&currentstringwhere);

	read_string(&str, &len);

	curtok = token_string;

	for(;;){
		int c = nextchar();
		if(c == '"'){
			/* "abc" "def"
			 *       ^
			 */
			char *new, *alloc;
			size_t newlen;

			read_string(&new, &newlen);

			alloc = umalloc(newlen + len);

			memcpy(alloc, str, len);
			memcpy(alloc + len - 1, new, newlen);

			free(str);
			free(new);

			str = alloc;
			len += newlen - 1;
		}else{
			ungetchar(c);
			break;
		}
	}

	currentstring    = str;
	currentstringlen = len;
	currentstringwide = is_wide;
}

static void cc1_read_quoted_char(const int is_wide)
{
	int multichar;
	char *p;
	long ch = read_quoted_char(bufferpos, &p, &multichar, /*256*/!is_wide);

	if(multichar){
		if(ch & (~0UL << (CHAR_BIT * type_primitive_size(type_int))))
			cc1_warn_at(NULL, multichar_toolarge, "multi-char constant too large");
		else
			cc1_warn_at(NULL, multichar, "multi-char constant");
	}

	currentval.val.i = ch;
	currentval.suffix = 0;
	curtok = is_wide ? token_integer : token_character;

	update_bufferpos(p);
}

static void read_number(const int first)
{
	char *const num_start = bufferpos - 1;
	enum base mode;

	if(first == '0'){
		int next = *bufferpos;

		switch(tolower(next)){
			case 'x':
				mode = HEX;
				update_bufferpos(bufferpos + 1);
				break;
			case 'b':
				cc1_warn_at(NULL, binary_literal, "binary literals are an extension");
				mode = BIN;
				update_bufferpos(bufferpos + 1);
				break;

			default:
				mode = OCT;

				if(isoct(next)){
					/* fine */
				}else if(isdigit(next)){
					die_at(NULL, "invalid oct character '%c'", next);
				}else{
					/* just zero */
					update_bufferpos(num_start);
				}
				break;
		}
	}else{
		mode = DEC;
		update_bufferpos(num_start);
	}

	if(*bufferpos != '.'){
		int just_zero = (mode == OCT && !isdigit(*bufferpos));

		if(just_zero){
			currentval.val.i = 0;
			currentval.suffix = VAL_OCTAL;
		}else{
			read_integer(mode);
		}
	}

	if(*bufferpos == '.' || peeknextchar() == '.'){
		char *new;

		if(mode != DEC)
			warn_at_print_error(NULL, "invalid prefix on floating literal");

		currentval.val.f = strtold(num_start, &new);
		currentval.suffix = VAL_FLOATING;
		update_bufferpos(new);
	}

	read_suffix();

	curtok = (currentval.suffix & VAL_FLOATING ? token_floater : token_integer);
}

void nexttoken()
{
	int c;

	if(buffereof){
		/* delay this until we are asked for token_eof */
		parse_finished = 1;
		curtok = token_eof;
		return;
	}

	if((c = getungetchar()) == EOF){
		c = nextchar();

		loc_tok.chr = loc_now.chr - 1;

		if(c == EOF){
			curtok = token_eof;
			return;
		}
	}

	if(isdigit(c) || (c == '.' && isdigit(peeknextchar()))){
		read_number(c);
		return;
	}

	if(c == '.'){
		curtok = token_dot;

		if(peeknextchar() == '.'){
			nextchar();
			if(peeknextchar() == '.'){
				nextchar();
				curtok = token_elipsis;
			}
			/* else leave it at token_dot and next as token_dot;
			 * parser will get an error */
		}
		return;
	}

	switch(c == 'L' ? peeknextchar() : 0){
		case '"':
			/* wchar_t string */
			nextchar();
			read_string_multiple(1);
			return;
		case '\'':
			nextchar();
			cc1_read_quoted_char(1);
			return;
	}

	if(isalpha(c) || c == '_' || c == '$'){
		unsigned int len = 1;
		char *const start = bufferpos - 1; /* regrab the char we switched on */
		const struct keyword *k;

		do{ /* allow numbers */
			c = peeknextchar();
			if(isalnum(c) || c == '_' || c == '$'){
				nextchar();
				len++;
			}else
				break;
		}while(1);

		/* check for a built in statement - while, if, etc */
		for(k = keywords; k->str; k++)
			if(k->mode & keyword_mode && strlen(k->str) == len && !strncmp(k->str, start, len)){
				curtok = k->tok;
				return;
			}


		/* not found, wap into currentspelling */
		free(currentspelling);
		currentspelling = umalloc(len + 1);

		strncpy(currentspelling, start, len);
		currentspelling[len] = '\0';
		curtok = token_identifier;
		return;
	}

	switch(c){
		case '"':
			read_string_multiple(0);
			break;

		case '\'':
			cc1_read_quoted_char(0);
			break;

		case '(':
			curtok = token_open_paren;
			break;
		case ')':
			curtok = token_close_paren;
			break;
		case '+':
			if(peeknextchar() == '+'){
				nextchar();
				curtok = token_increment;
			}else{
				curtok = token_plus;
			}
			break;
		case '-':
			switch(peeknextchar()){
				case '-':
					nextchar();
					curtok = token_decrement;
					break;
				case '>':
					nextchar();
					curtok = token_ptr;
					break;

				default:
					curtok = token_minus;
			}
			break;
		case '*':
			curtok = token_multiply;
			break;
		case '/':
			if(peeknextchar() == '*'){
				in_comment = 1;

				for(;;){
					int c = rawnextchar();
					if(c == '*' && *bufferpos == '/'){
						rawnextchar(); /* eat the / */
						nexttoken();

						in_comment = 0;
						return;
					}
				}
				die_at(NULL, "No end to comment");
				return;
			}else if(peeknextchar() == '/'){
				tokenise_next_line();
				nexttoken();
				return;
			}
			curtok = token_divide;
			break;
		case '%':
			curtok = token_modulus;
			break;

		case '<':
			if(peeknextchar() == '='){
				nextchar();
				curtok = token_le;
			}else if(peeknextchar() == '<'){
				nextchar();
				curtok = token_shiftl;
			}else{
				curtok = token_lt;
			}
			break;

		case '>':
			if(peeknextchar() == '='){
				nextchar();
				curtok = token_ge;
			}else if(peeknextchar() == '>'){
				nextchar();
				curtok = token_shiftr;
			}else{
				curtok = token_gt;
			}
			break;

		case '=':
			if(peeknextchar() == '='){
				nextchar();
				curtok = token_eq;
			}else
				curtok = token_assign;
			break;

		case '!':
			if(peeknextchar() == '='){
				nextchar();
				curtok = token_ne;
			}else
				curtok = token_not;
			break;

		case '&':
			if(peeknextchar() == '&'){
				nextchar();
				curtok = token_andsc;
			}else
				curtok = token_and;
			break;

		case '|':
			if(peeknextchar() == '|'){
				nextchar();
				curtok = token_orsc;
			}else
				curtok = token_or;
			break;

		case ',':
			curtok = token_comma;
			break;

		case ':':
			curtok = token_colon;
			break;

		case '?':
			curtok = token_question;
			break;

		case ';':
			curtok = token_semicolon;
			break;

		case ']':
			curtok = token_close_square;
			break;

		case '[':
			curtok = token_open_square;
			break;

		case '}':
			curtok = token_close_block;
			break;

		case '{':
			curtok = token_open_block;
			break;

		case '~':
			curtok = token_bnot;
			break;

		case '^':
			curtok = token_xor;
			break;

		default:
			die_at(NULL, "unknown character %c 0x%x %d", c, c, buffereof);
			curtok = token_unknown;
	}

	if(curtok_is_xequal() && peeknextchar() == '='){
		nextchar();
		curtok = curtok_to_xequal(); /* '+' '=' -> "+=" */
	}
}
