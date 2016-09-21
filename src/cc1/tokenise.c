#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <stdarg.h>
#include <limits.h>
#include <assert.h>
#include <errno.h>

#include "../util/util.h"
#include "tokenise.h"
#include "../util/alloc.h"
#include "../util/str.h"
#include "../util/escape.h"
#include "str.h"
#include "cc1.h"
#include "cc1_where.h"
#include "btype.h"
#include "fopt.h"

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

	KEYWORD(KW_ALL, __label__),

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

struct cstring *currentstring = NULL; /* a string literal */
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
	if(ep == l){
		cc1_warn_at(NULL, cpp_line_parsing,
				"couldn't parse number for #line directive (%s)", ep);
		return;
	}

	if(lno < 0){
		cc1_warn_at(NULL, cpp_line_parsing, "negative #line directive argument");
		return;
	}

	loc_now.line = lno - 1; /* inc'd below */

	ep = str_spc_skip(ep);

	switch(*ep){
		case '"':
			{
				char *p = str_quotefin(++ep);
				if(!p){
					cc1_warn_at(NULL, cpp_line_parsing,
							"no terminating quote to #line directive (%s)",
							l);
					return;
				}

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
			cc1_warn_at(NULL, cpp_line_parsing,
					"expected '\"' or nothing after #line directive (%s)",
					ep);
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

static ucc_wur char *tokenise_read_line(void)
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
		if(cc1_fopt.show_line)
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

static void tokenise_next_line(void)
{
	char *new = tokenise_read_line();

	if(new)
		SET_CURRENT_LINE_STR(ustrdup(new));

	if(buffer){
		if((cc1_fopt.show_line) == 0)
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

	if(cc1_fopt.track_initial_fnam)
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

static int rawnextchar(void)
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

static int char_is_cspace(int ch)
{
	/* C allows ^L aka '\f' anywhere in the code */
	return isspace(ch) || ch == '\f';
}

static int nextchar(void)
{
	int c;
	do
		c = rawnextchar();
	while(char_is_cspace(c));
	return c;
}

static int peeknextchar(void)
{
	/* doesn't ignore isspace() */
	if(!bufferpos)
		tokenise_next_line();

	if(buffereof)
		return EOF;

	return *bufferpos;
}

static void skip_to_end_of_num(void)
{
	char *p;

	for(p = bufferpos;
			isalnum(*p) || *p == '.';
			p++);

	update_bufferpos(p);
}

static void read_integer(const enum base mode)
{
	char *end;
	int of; /*verflow*/
	where loc;

	where_cc1_current(&loc);

	switch(mode){
		case BIN: currentval.suffix = VAL_BIN; break;
		case HEX: currentval.suffix = VAL_HEX; break;
		case OCT: currentval.suffix = VAL_OCTAL; break;
		case DEC: currentval.suffix = 0; break;
	}

	currentval.val.i = char_seq_to_ullong(bufferpos, &end, /*apply_limit*/0, mode, &of);

	if(of){
		/* force unsigned long long ULLONG_MAX */
		cc1_warn_at(&loc, overflow,
				"overflow parsing integer, truncating to unsigned long long");

		currentval.val.i = NUMERIC_T_MAX;
		currentval.suffix = VAL_LLONG | VAL_UNSIGNED;
	}

	if(end == bufferpos){
		parse_had_error = 1;
		warn_at_print_error(NULL,
				"%s-number expected",
				base_to_str(mode));
		return;
	}

	update_bufferpos(end);
}

static void read_suffix_float_exp(void)
{
	numeric mantissa;
	int powmul;
	int just_read = nextchar();

	assert(tolower(just_read) == 'e');

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
		skip_to_end_of_num();
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
	const int fp = (currentval.suffix & VAL_FLOATING
			|| tolower(peeknextchar()) == 'e');

	/* handle floating XeY */
	if(fp){
		read_suffix_float();
	}else{
		read_suffix_int();
	}


	if(isalpha(peeknextchar()) || peeknextchar() == '.'){
		warn_at_print_error(NULL,
				"invalid suffix on %s constant (%c)",
				fp ? "floating point" : "integer",
				peeknextchar());

		parse_had_error = 1;
		skip_to_end_of_num();
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

static int curtok_is_xequal(void)
{
	return curtok_to_xequal() != token_unknown;
}

static void handle_escape_warn_err(int warn, int err, void *ctx)
{
	extern int parse_had_error;
	const where *loc = ctx;
	where loc_;

	if(!loc){
		where_cc1_current(&loc_);
		loc = &loc_;
	}

	switch(err){
		case 0:
			break;
		case EILSEQ:
			warn_at_print_error(loc, "empty escape sequence");
			parse_had_error = 1;
			break;
		default:
			assert(0 && "unhandled escape error");
	}
	switch(warn){
		case 0:
			break;
		case E2BIG:
			cc1_warn_at(loc, char_toolarge, "ignoring extraneous characters in literal");
			break;
		case ERANGE:
			warn_at_print_error(loc, "escape sequence out of range");
			parse_had_error = 1;
			break;
		case EINVAL:
			warn_at_print_error(loc, "invalid escape character");
			parse_had_error = 1;
			break;
		default:
			assert(0 && "unhandled escape warning");
	}
}

static struct cstring *read_string(int is_wide)
{
	const char *start = bufferpos;
	const char *end = str_quotefin((char *)start);
	struct cstring *ret;

	if(!end){
		const char *empty = "";

		char *p;
		if((p = strchr(bufferpos, '\n')))
			*p = '\0';
		warn_at_print_error(NULL, "no terminating quote to string");
		parse_had_error = 1;

		start = empty;
		end = empty + 1;
	}

	ret = cstring_new_raw_from_ascii(start, end - 1);

	update_bufferpos(bufferpos + ret->count);

	cstring_escape(ret, is_wide, handle_escape_warn_err, NULL);

	return ret;
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

static void read_string_multiple(int is_wide)
{
	struct cstring *cstr;

	where_cc1_current(&currentstringwhere);

	cstr = read_string(is_wide);

	curtok = token_string;

	for(;;){
		/* look for '"' or "L\"" */
		int c = nextchar();

		if(c == '"' || (c == 'L' && *bufferpos == '"')){
			/* "abc" "def"
			 *       ^
			 */
			struct cstring *appendstr;

			if(c == 'L'){
				is_wide = 1;
				bufferpos++;
			}

			appendstr = read_string(is_wide);

			cstring_append(cstr, appendstr);
			cstring_free(appendstr);
		}else{
			ungetchar(c);
			break;
		}
	}

	currentstring = cstr;
}

static void read_char(int is_wide)
{
	char *begin = bufferpos;
	char *end = char_quotefin(begin);
	int ch = 0;
	int multichar = 0;

	if(end){
		const size_t len = (end - begin);
		int warn, err;
		char *endesc;

		warn = err = 0;
		ch = escape_char(begin, end, &endesc, is_wide, &multichar, &warn, &err);

		if(endesc != end){
			warn_at_print_error(NULL, "invalid characters in literal");
			parse_had_error = 1;
		}

		if(multichar){
			if(ch & (~0UL << (CHAR_BIT * type_primitive_size(type_int))))
				cc1_warn_at(NULL, char_toolarge, "multi-char constant too large");
			else
				cc1_warn_at(NULL, multichar, "multi-char constant");
		}else if(len == 0){
			warn_at_print_error(NULL, "empty char constant");
			parse_had_error = 1;
			goto out;
		}

		handle_escape_warn_err(warn, err, NULL);
	}else{
		warn_at_print_error(NULL, "no terminating quote to character literal");
		parse_had_error = 1;
	}

out:
	if(end)
		update_bufferpos(end + 1);

	if(is_wide || multichar)
		currentval.val.i = (int)ch;
	else
		currentval.val.i = (char)ch; /* ensure we sign extend from char */

	currentval.suffix = 0;
	curtok = is_wide ? token_integer : token_character;
}

static void read_number(const int first)
{
	char *const num_start = bufferpos - 1;
	char *p;
	enum base mode;
	int just_zero = 0;

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
					just_zero = 1;
				}
				break;
		}
	}else{
		mode = DEC;
		update_bufferpos(num_start);
	}

	/* check for '.' after prefix handling:
	 * may be a hex-float constant */
	for(p = bufferpos; (mode == HEX ? isxdigit : isdigit)(*p); p++);

	if(*p == '.' || (mode == HEX && *p == 'p')){
		char *new;
		int bad_prefix = 0;

		currentval.val.f = strtold(num_start, &new);
		currentval.suffix = VAL_FLOATING;
		update_bufferpos(new);

		switch(mode){
			case DEC:
				break;

			case HEX:
				/* check for exponent */
				assert(!strncmp(num_start, "0x", 2));
				for(p = num_start + 2; isxdigit(*p) || *p == '.'; p++);

				if(tolower(*p) != 'p'){
					warn_at_print_error(NULL, "floating literal requires exponent");
					parse_had_error = 1;
					skip_to_end_of_num();
				}
				break;

			case OCT:
				if(!just_zero)
					bad_prefix = 1;
				break;
			case BIN:
				bad_prefix = 1;
				break;
		}

		if(bad_prefix){
			warn_at_print_error(NULL, "invalid prefix on floating literal");
			parse_had_error = 1;
			skip_to_end_of_num();
		}

	}else{
		if(just_zero){
			update_bufferpos(p);
			currentval.val.i = 0;
			currentval.suffix = VAL_OCTAL;
		}else{
			read_integer(mode);
		}
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
			read_char(1);
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
			read_char(0);
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
