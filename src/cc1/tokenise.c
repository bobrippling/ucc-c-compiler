#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <stdarg.h>
#include <limits.h>

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

#define KEYWORD(x) { #x, token_ ## x }

#define KEYWORD__(x, t) \
	{ "__" #x,      t },  \
	{ "__" #x "__", t }

#define KEYWORD__ALL(x) KEYWORD(x), KEYWORD__(x, token_ ## x)

struct statement
{
	const char *str;
	enum token tok;
} statements[] = {
#ifdef BRITISH
	{ "perchance", token_if      },
	{ "otherwise", token_else    },

	{ "what_about",        token_switch  },
	{ "perhaps",           token_case    },
	{ "on_the_off_chance", token_default },

	{ "splendid",    token_break    },
	{ "goodday",     token_return   },
	{ "as_you_were", token_continue },

	{ "tallyho",     token_goto     },
#else
	KEYWORD(if),
	KEYWORD(else),

	KEYWORD(switch),
	KEYWORD(case),
	KEYWORD(default),

	KEYWORD(break),
	KEYWORD(return),
	KEYWORD(continue),

	KEYWORD(goto),
#endif

	KEYWORD(do),
	KEYWORD(while),
	KEYWORD(for),

	KEYWORD__ALL(asm),

	KEYWORD(void),
	KEYWORD(char),
	KEYWORD(int),
	KEYWORD(short),
	KEYWORD(long),
	KEYWORD(float),
	KEYWORD(double),
	KEYWORD(_Bool),

	KEYWORD(auto),
	KEYWORD(static),
	KEYWORD(extern),
	KEYWORD(register),

	KEYWORD__ALL(inline),
	KEYWORD(_Noreturn),

	KEYWORD__ALL(const),
	KEYWORD__ALL(volatile),
	KEYWORD__ALL(restrict),

	KEYWORD__ALL(signed),
	KEYWORD__ALL(unsigned),

	KEYWORD(typedef),
	KEYWORD(struct),
	KEYWORD(union),
	KEYWORD(enum),

	KEYWORD(_Alignof),
	KEYWORD__(alignof, token__Alignof),
	KEYWORD(_Alignas),
	KEYWORD__(alignas, token__Alignas),

	KEYWORD(__builtin_va_list),

	KEYWORD(sizeof),
	KEYWORD(_Generic),
	KEYWORD(_Static_assert),

	KEYWORD(__extension__),

	KEYWORD__ALL(typeof),

	KEYWORD__(attribute, token_attribute),
};

static tokenise_line_f *in_func;
int buffereof = 0;
static int parse_finished = 0;

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

static void read_number(const enum base mode)
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

	/* -1, since we've already eaten the first numeric char */
	loc_now.chr += end - bufferpos - 1;
	bufferpos = end;

	/* accept either 'U' 'L' or 'LL' as atomic parts (i.e. not LUL) */
	/* fine using nextchar() since we peeknextchar() first */
	{
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

	bufferpos += size;
	loc_now.chr += size;
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
	long ch = read_quoted_char(bufferpos, &bufferpos, &multichar);

	if(multichar){
		if(ch & (~0UL << (CHAR_BIT * type_primitive_size(type_int))))
			warn_at(NULL, "multi-char constant too large");
		else
			warn_at(NULL, "multi-char constant");
	}

	currentval.val.i = ch;
	currentval.suffix = 0;
	curtok = is_wide ? token_integer : token_character;
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
		char *const num_start = bufferpos - 1;
		enum base mode;

		if(c == '0'){
			/* note the '0' */
			loc_now.chr++;

			switch(tolower(c = peeknextchar())){
				case 'x':
					mode = HEX;
					nextchar();
					break;
				case 'b':
					mode = BIN;
					nextchar();
					break;
				default:
					if(!isoct(c)){
						if(isdigit(c))
							die_at(NULL, "invalid oct character '%c'", c);
						else
							mode = DEC; /* just zero */

						bufferpos--; /* have the zero */
						loc_now.chr--;
					}else{
						mode = OCT;
					}
					break;
			}
		}else{
			mode = DEC;
			bufferpos--; /* rewind */
		}

		if(c != '.')
			read_number(mode);

#if 0
		if(tolower(peeknextchar()) == 'e'){
			/* 5e2 */
			int n = currentval.val;

			if(!isdigit(peeknextchar())){
				curtok = token_unknown;
				return;
			}
			read_number();

			currentval.val = n * pow(10, currentval.val);
			/* cv = n * 10 ^ cv */
		}
#endif

		if(c == '.' || peeknextchar() == '.'){
			/* floating point */

			currentval.val.f = strtold(num_start, &bufferpos);

			if(toupper(peeknextchar()) == 'F'){
				currentval.suffix = VAL_FLOAT;
				nextchar();
			}else if(toupper(peeknextchar()) == 'L'){
				currentval.suffix = VAL_LDOUBLE;
				nextchar();
			}else{
				currentval.suffix = VAL_DOUBLE;
			}

			curtok = token_floater;

		}else{
			curtok = token_integer;
		}

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
		unsigned int len = 1, i;
		char *const start = bufferpos - 1; /* regrab the char we switched on */

		do{ /* allow numbers */
			c = peeknextchar();
			if(isalnum(c) || c == '_' || c == '$'){
				nextchar();
				len++;
			}else
				break;
		}while(1);

		/* check for a built in statement - while, if, etc */
		for(i = 0; i < sizeof(statements) / sizeof(statements[0]); i++)
			if(strlen(statements[i].str) == len && !strncmp(statements[i].str, start, len)){
				curtok = statements[i].tok;
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
