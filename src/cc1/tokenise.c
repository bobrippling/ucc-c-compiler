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

#define isoct(x) ('0' <= (x) && (x) < '8')

struct stmtement
{
	const char *str;
	enum token tok;
} stmtements[] = {
#ifdef BRITISH
	{ "perchance", token_if      },
	{ "otherwise", token_else    },

	{ "what_about",        token_switch  },
	{ "perhaps",           token_case    },
	{ "on_the_off_chance", token_default },

	{ "splendid", token_break   },
	{ "goodday",  token_return  },

	{ "tallyho", token_goto    },
#else
	{ "if",      token_if      },
	{ "else",    token_else    },

	{ "switch",  token_switch  },
	{ "case",    token_case    },
	{ "default", token_default },

	{ "break",   token_break   },
	{ "return",  token_return  },

	{ "goto",    token_goto    },
#endif

	{ "do",      token_do      },
	{ "while",   token_while   },
	{ "for",     token_for     },

	{ "char",    token_char    },
	{ "int",     token_int     },
	{ "void",    token_void    },
	{ "extern",  token_extern  },
	{ "const",   token_const   },
	{ "static",  token_static  },
	{ "signed",  token_signed  },
	{ "unsigned",token_unsigned},
	{ "auto",    token_auto    },
	{ "typedef", token_typedef },
	{ "struct",  token_struct  },
	{ "enum",    token_enum    },

	{ "sizeof",  token_sizeof  }
};

static FILE *infile;
char *current_fname;

static char *buffer, *bufferpos;
static int buffereof = 0;
static int ungetch = EOF;

/* -- */
enum token curtok;

intval currentval = { 0, 0 }; /* an integer literal */

char *currentspelling = NULL; /* e.g. name of a variable */

char *currentstring   = NULL; /* a string literal */
int   currentstringlen = 0;

/* -- */
int current_line = 0;
int current_chr  = 0;


static void tokenise_read_line()
{
	char *l;

	if(buffereof)
		return;

	if(buffer){
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

		/* format is # [0-9] "filename" ([0-9])* */
		if(sscanf(l, "# %d ", &lno) == 1){
			char *p = strchr(l, '"');
			char *fin;

			fin = p + 1;
			for(;;){
				fin = strchr(fin, '"');

				if(!fin)
					die("no terminating quote for pre-proc info");

				if(fin[-1] != '\\')
					break;
				fin++;
			}

			free(current_fname);
			current_fname = ustrdup2(p + 1, fin);

			current_line = lno - 1; /* inc'd below */

			tokenise_read_line();
			return;
		}

		current_line++;
		current_chr = -1;
	}

	bufferpos = buffer = l;
}

void tokenise_set_file(FILE *f, const char *nam)
{
	infile = f;
	current_fname = ustrdup(nam);
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
	return *bufferpos;
}

int readnumber(char c)
{
	enum { DEC, HEX, OCT, BIN } mode = c == '0' ? OCT : DEC;
	unsigned long lval;

	lval = c - '0';
	c = peeknextchar();

	if((c == 'x' || c == 'b') && mode == OCT){
		mode = c == 'x' ? HEX : BIN;
		nextchar();
		c = peeknextchar();
	}

#define READ_NUM(test, base) \
			do{ \
				while(c == '_'){ \
					nextchar(); \
					c = peeknextchar(); \
				} \
				if(test){ \
					lval = base * lval + c - '0'; \
					nextchar(); \
				}else{ \
					break; \
				} \
				c = peeknextchar(); \
			}while(1);

	switch(mode){
		case BIN:
			READ_NUM(c == '0' || c == '1', 2);
			break;

		case DEC:
			READ_NUM(isdigit(c), 10);
			break;

		case OCT:
			READ_NUM(isoct(c), 010);
			break;

		case HEX:
		{
			int charsread = 0;
			do{
				if(isxdigit(c)){
					c = tolower(c);
					lval = 0x10 * lval + (isdigit(c) ? c - '0' : 10 + c - 'a');
					nextchar();
				}else{
					break;
				}
				c = peeknextchar();
				while(c == '_'){
					nextchar();
					c = peeknextchar();
				}
				charsread++;
			}while(1);

			if(charsread < 1){
				warn_at(NULL, "need proper hex number!");
				curtok = token_unknown;
				return 1;
			}
			break;
		}
	}

	currentval.val = lval;
	currentval.suffix = 0;

	while(1)
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
				return 0;
		}
}

enum token curtok_to_xequal()
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
		MAP(token_shiftl);
		MAP(token_shiftr);
#undef MAP

		default:
			break;
	}
	return token_unknown;
}

int curtok_is_xequal()
{
	return curtok_to_xequal(curtok) != token_unknown;
}

void read_string(char **sptr, int *plen)
{
	char *end = strchr(bufferpos, '"'), *const start = bufferpos;
	int size;

	if(!end){
		if((end = strchr(bufferpos, '\n')))
			*end = '\0';
		die_at(NULL, "Couldn't find terminating quote to \"%s\"", bufferpos);
	}

	if(end > bufferpos)
		while(end && end[-1] == '\\') /* FIXME: "hi\\" */
			end = strchr(end + 1, '"');

	size = end - start + 1;

	*sptr = umalloc(size);
	*plen = size;

	strncpy(*sptr, start, size);
	(*sptr)[size-1] = '\0';

	escapestring(*sptr, plen);

	bufferpos += size;
}

void nexttoken()
{
	int c;

	if(buffereof){
		curtok = token_eof;
		return;
	}

	if(ungetch != EOF){
		c = ungetch;
		ungetch = EOF;
	}else{
		c = nextchar();
	}

	if(c == EOF){
		curtok = token_eof;
		return;
	}

	if(isdigit(c)){
		if(readnumber(c)){
			curtok = token_unknown;
			return;
		}

		if(tolower(peeknextchar()) == 'e'){
			int n = currentval.val;

			/* 5e2 */
			nextchar();

			if(!isdigit(c = nextchar()) || readnumber(c)){
				curtok = token_unknown;
				return;
			}

			currentval.val = n * pow(10, currentval.val);
			/* cv = n * 10 ^ cv */
		}

		curtok = token_integer;
		return;
	}

	if(c == '.'){
		if(peeknextchar() == '.'){
			nextchar();
			if(peeknextchar() == '.'){
				nextchar();
				curtok = token_elipsis;
			}else{
				curtok = token_unknown;
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

		/* check for a built in stmtement - while, if, etc */
		for(i = 0; i < sizeof(stmtements) / sizeof(stmtements[0]); i++)
			if(strlen(stmtements[i].str) == len && !strncmp(stmtements[i].str, start, len)){
				curtok = stmtements[i].tok;
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
			switch(c){
				case EOF:
					warn_at(NULL, "Invalid character");
					curtok = token_unknown;
					return;

				case '\\':
				{
					int save;

					/* special parsing */
					c = escapechar((save = nextchar()));

					if(c == -1){
						warn_at(NULL, "warning: ignoring escape character before '%c'", save);
						c = save;
					}
					break;
				}
			}

			currentval.val = c;
			currentval.suffix = 0;
			if((c = nextchar()) == '\'')
				curtok = token_character;
			else{
				die_at(NULL, "Invalid character token\n"
						"(expected single quote, not '%c')", c);
				curtok = token_unknown;
				return;
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
				die_at(NULL, "No end to comment");
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
			curtok = token_unknown;
	}

	if(curtok_is_xequal() && peeknextchar() == '='){
		nextchar();
		curtok = curtok_to_xequal(); /* '+' '=' -> "+=" */
	}
}
