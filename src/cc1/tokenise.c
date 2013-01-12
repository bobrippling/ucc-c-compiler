#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <stdarg.h>

#include "../util/util.h"
#include "data_structs.h"
#include "tokenise.h"
#include "../util/alloc.h"
#include "../util/util.h"
#include "str.h"
#include "cc1.h"

#define KEYWORD(x) { #x, token_ ## x }

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

	KEYWORD(asm),
	{ "__asm",    token_asm },
	{ "__asm__",  token_asm },

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

	KEYWORD(inline),
	KEYWORD(_Noreturn),

	KEYWORD(const),
	KEYWORD(volatile),
	KEYWORD(restrict),

	KEYWORD(signed),
	KEYWORD(unsigned),

	KEYWORD(typedef),
	KEYWORD(struct),
	KEYWORD(union),
	KEYWORD(enum),

	KEYWORD(sizeof),
	KEYWORD(_Generic),
	KEYWORD(_Static_assert),

	KEYWORD(typeof),
	{ "__typeof",    token_typeof },
	{ "__typeof__",  token_typeof },

	{ "__attribute",   token_attribute },
	{ "__attribute__", token_attribute }
};

static FILE *infile;
char *current_fname;
int buffereof = 0;
int current_fname_used;
int parse_finished = 0;

static char *buffer, *bufferpos;
static int ungetch = EOF;

static struct line_list
{
	char *line;
	struct line_list *next;
} *store_lines, **store_line_last = &store_lines;

/* -- */
enum token curtok, curtok_uneat;

intval currentval = { 0, 0 }; /* an integer literal */

char *currentspelling = NULL; /* e.g. name of a variable */

char *currentstring   = NULL; /* a string literal */
int   currentstringlen = 0;

/* -- */
int current_line = 0;
int current_chr  = 0;
char *current_line_str = NULL;
int current_line_str_used = 0;

static void add_store_line(char *l)
{
	struct line_list *new = umalloc(sizeof *new);
	new->line = l;

	*store_line_last = new;
	store_line_last = &new->next;
}

static void tokenise_read_line()
{
	char *l;

	if(buffereof)
		return;

	if(buffer){
		if((fopt_mode & FOPT_SHOW_LINE) == 0)
			free(buffer);
		buffer = NULL;
	}

	l = fline(infile);
	if(!l){
		if(feof(infile))
			buffereof = 1;
		else
			die("read():");
	}else{
		/* check for preprocessor line info */
		int lno;

		/* but first - add to store_lines */
		if(fopt_mode & FOPT_SHOW_LINE)
			add_store_line(l);

		/* format is # [0-9] "filename" ([0-9])* */
		if(sscanf(l, "# %d", &lno) == 1){
			char *p = strchr(l, '"');
			char *fin;

			if(p){
				fin = p + 1;
				for(;;){
					fin = strchr(fin, '"');

					if(!fin)
						die("no terminating quote for pre-proc info");

					if(fin[-1] != '\\')
						break;
					fin++;
				}

				if(!current_fname_used)
					free(current_fname); /* else it's been taken by one or more where_new()s */

				current_fname = ustrdup2(p + 1, fin);
				current_fname_used = 0;
			}else{
				/* check there's nothing left */
				for(p = l + 2; isdigit(*p); p++);
				for(; isspace(*p); p++);

				if(*p != '\0')
					die("extra text after # 0-9: \"%s\"", p);
			}

			current_line = lno - 1; /* inc'd below */

			tokenise_read_line();

			return;
		}

		current_line++;
		current_chr = -1;
	}

	if(l){
		if(!current_line_str_used)
			free(current_line_str);
		current_line_str = ustrdup(l);
		current_line_str_used = 0;
	}

	bufferpos = buffer = l;
}

void tokenise_set_file(FILE *f, const char *nam)
{
	infile = f;

	if(!current_fname_used)
		free(current_fname);
	current_fname = ustrdup(nam);
	current_fname_used = 0;

	if(!current_line_str_used)
		free(current_line_str);
	current_line_str = NULL;
	current_line_str_used = 0;

	current_line = 0;
	buffereof = 0;
	nexttoken();
}

static int rawnextchar()
{
	if(buffereof)
		return EOF;

	while(!bufferpos || !*bufferpos){
		tokenise_read_line();
		if(buffereof)
			return EOF;
	}

	current_chr++;
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
		tokenise_read_line();

	if(buffereof)
		return EOF;

	return *bufferpos;
}

void read_number(enum base mode)
{
	int read_suffix = 1;
	int nlen;

	char_seq_to_iv(bufferpos, &currentval, &nlen, mode);

	bufferpos += nlen;
	currentval.suffix = 0;

	while(read_suffix)
		switch(peeknextchar()){
			case 'U':
				currentval.suffix = VAL_UNSIGNED;
				nextchar();
				break;
			case 'L':
				currentval.suffix = VAL_LONG;
				nextchar();
				ICE("TODO: long integer suffix");
				break;
			default:
				read_suffix = 0;
		}
}

static enum token curtok_to_xequal()
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
	return curtok_to_xequal(curtok) != token_unknown;
}

void read_string(char **sptr, int *plen)
{
	char *const start = bufferpos;
	char *const end = terminating_quote(start);
	int size;

	if(!end){
		char *p;
		if((p = strchr(bufferpos, '\n')))
			*p = '\0';
		DIE_AT(NULL, "Couldn't find terminating quote to \"%s\"", bufferpos);
	}

	size = end - start + 1;

	*sptr = umalloc(size);
	*plen = size;

	strncpy(*sptr, start, size);
	(*sptr)[size-1] = '\0';

	escape_string(*sptr, plen);

	bufferpos += size;
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

	if(ungetch != EOF){
		c = ungetch;
		ungetch = EOF;
	}else{
		c = nextchar(); /* no numbers, no more peeking */
		if(c == EOF){
			curtok = token_eof;
			return;
		}
	}

	if(isdigit(c)){
		enum base mode;

		if(c == '0'){
			switch((c = nextchar())){
				case 'x':
					mode = HEX;
					break;
				case 'b':
					mode = BIN;
					break;
				default:
					if(!isoct(c)){
						if(isdigit(c))
							DIE_AT(NULL, "invalid oct character '%c'", c);
						else
							mode = DEC; /* just zero */
					}else{
						mode = OCT;
					}

					bufferpos--; /* rewind over c */
					break;
			}
		}else{
			mode = DEC;
			bufferpos--; /* rewind */
		}

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

		if(peeknextchar() == '.'){
			/* double or float */
			int parts[2];
			nextchar();

			parts[0] = currentval.val;
			read_number(DEC);
			parts[1] = currentval.val;

			if(peeknextchar() == 'f'){
				nextchar();
				/* FIXME: set currentval.suffix instead of token_{int,float,double} ? */
				curtok = token_float;
			}else{
				curtok = token_double;
			}

			ICE("TODO: float/double repr (%d.%d)", parts[0], parts[1]);
		}else{
			curtok = token_integer;
		}

		return;
	}

	if(c == '.'){
		if(peeknextchar() == '.'){
			nextchar();
			if(peeknextchar() == '.'){
				nextchar();
				curtok = token_elipsis;
			}else{
				DIE_AT(NULL, "unknown token \"..\"\n");
			}
			return;
		}else{
			curtok = token_dot;
			return;
		}
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
		{
			/* TODO: read in "hello\\" - parse string char by char, rather than guessing and escaping later */
			char *str;
			int len;

			read_string(&str, &len);

			curtok = token_string;

recheck:
			c = nextchar();
			if(c == '"'){
				char *new, *alloc;
				int newlen;

				read_string(&new, &newlen);

				alloc = umalloc(newlen + len);

				memcpy(alloc, str, len);
				memcpy(alloc + len - 1, new, newlen);

				free(str);
				free(new);

				str = alloc;
				len += newlen - 1;

				goto recheck;
			}else{
				if(ungetch != EOF)
					ICE("ungetch");
				ungetch = c;
			}

			currentstring    = str;
			currentstringlen = len;
			break;
		}

		case '\'':
		{
			c = rawnextchar();

			if(c == EOF){
				DIE_AT(NULL, "Invalid character");
			}else if(c == '\\'){
				char esc = peeknextchar();

				if(esc == 'x' || esc == 'b' || isoct(esc)){

					if(esc == 'x' || esc == 'b')
						nextchar();

					read_number(esc == 'x' ? HEX : esc == 'b' ? BIN : OCT);

					if(currentval.suffix)
						DIE_AT(NULL, "invalid character sequence: suffix given");

					if(currentval.val > 0xff)
						warn_at(NULL, 1, "invalid character sequence: too large (parsed 0x%lx)", currentval.val);

					c = currentval.val;
				}else{
					/* special parsing */
					c = escape_char(esc);

					if(c == -1)
						DIE_AT(NULL, "invalid escape character '%c'", esc);

					nextchar();
				}
			}

			currentval.val = c;
			currentval.suffix = 0;

			if((c = nextchar()) == '\''){
				curtok = token_character;
			}else{
				DIE_AT(NULL, "no terminating \"'\" for character (got '%c')", c);
			}

			break;
		}


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
				/* comment */
				for(;;){
					int c = rawnextchar();
					if(c == '*' && *bufferpos == '/'){
						rawnextchar(); /* eat the / */
						nexttoken();
						return;
					}
				}
				DIE_AT(NULL, "No end to comment");
				return;
			}else if(peeknextchar() == '/'){
				tokenise_read_line();
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
			DIE_AT(NULL, "unknown character %c 0x%x %d", c, c, buffereof);
			curtok = token_unknown;
	}

	if(curtok_is_xequal() && peeknextchar() == '='){
		nextchar();
		curtok = curtok_to_xequal(); /* '+' '=' -> "+=" */
	}
}
