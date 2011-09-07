#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "tree.h"
#include "tokenise.h"
#include "alloc.h"
#include "util.h"
#include "str.h"

#define isoct(x) ('0' <= (x) && (x) < '8')

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

	{ "splendid", token_break   },
	{ "goodday",  token_return  },
#else
	{ "if",      token_if      },
	{ "else",    token_else    },

	{ "switch",  token_switch  },
	{ "case",    token_case    },
	{ "default", token_default },

	{ "break",   token_break   },
	{ "return",  token_return  },
#endif

	{ "do",      token_do      },
	{ "while",   token_while   },
	{ "for",     token_for     },

	{ "char",    token_char    },
	{ "int",     token_int     },
	{ "void",    token_void    },
	{ "extern",  token_extern  },
	{ "const",   token_const   },

	{ "sizeof",  token_sizeof  }
};

static FILE *infile;
const char *currentfname;

static char *buffer, *bufferpos;
static int buffereof = 0;

/* -- */
enum token curtok;

int currentval = 0; /* an integer literal */

char *currentspelling = NULL; /* e.g. name of a variable */

char *currentstring   = NULL; /* a string literal */
int   currentstringlen = 0;

/* -- */
int currentline = 0;
int currentchar = 0;


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
		currentline++;
		currentchar = -1;
	}

	bufferpos = buffer = l;
}

void tokenise_set_file(FILE *f, const char *nam)
{
	infile = f;
	currentfname = nam;
	currentline = 0;
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

	currentchar++;
	return *bufferpos++;
}

static int nextchar()
{
	int c;
	do
		c = rawnextchar();
	while(isspace(c));
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

	currentval = c - '0';
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
					currentval = base * currentval + c - '0'; \
					nextchar(); \
				}else \
					break; \
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
					currentval = 0x10 * currentval + (isdigit(c) ? c - '0' : 10 + c - 'a');
					nextchar();
				}else
					break;
				c = peeknextchar();
				while(c == '_'){
					nextchar();
					c = peeknextchar();
				}
				charsread++;
			}while(1);

			if(charsread < 1){
				fputs("need proper hex number!\n", stderr);
				curtok = token_unknown;
				return 1;
			}
			break;
		}
	}

	return 0;
}

void nexttoken()
{
	int c;

	if(buffereof){
		curtok = token_eof;
		return;
	}

	c = nextchar();
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
			int n = currentval;

			/* 5e2 */
			nextchar();

			if(!isdigit(c = nextchar()) || readnumber(c)){
				curtok = token_unknown;
				return;
			}

			currentval = n * pow(10, currentval);
			/* cv = n * 10 ^ cv */
		}

#if token_read_debug
		printf(DEBUG_WORD"found integer: %d\n", currentval);
#endif
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

	if(isalpha(c) || c == '_'){
		unsigned int len = 1, i;
		char *const start = bufferpos - 1; /* regrab the char we switched on */

		do{ /* allow numbers */
			c = peeknextchar();
			if(isalnum(c) || c == '_'){
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
			/* read in the string - TODO: "hi" "there" */
			int size;
			char *end = strchr(bufferpos, '"'), *const start = bufferpos;

			if(end > bufferpos)
				while(end && end[-1] == '\\')
					end = strchr(end + 1, '"');

			if(!end){
				if((end = strchr(bufferpos, '\n')))
					*end = '\0';
				die_at(NULL, "Couldn't find terminating quote to \"%s\"", bufferpos);
			}

			size = end - start + 1;

			free(currentstring);
			currentstring = umalloc(size);
			currentstringlen = size;

			strncpy(currentstring, start, size);
			currentstring[size-1] = '\0';

			if(!escapestring(currentstring, &currentstringlen)){
				curtok = token_unknown;
				return;
			}

			curtok = token_string;

			bufferpos += size;
			break;
		}

		case '\'':
		{
			c = rawnextchar();
			switch(c){
				case EOF:
					fputs("Invalid character\n", stderr);
					curtok = token_unknown;
					return;

				case '\\':
				{
					int save;

					/* special parsing */
					c = escapechar((save = nextchar()));

					if(!c){
						fprintf(stderr, "Warning: Ignoring escape character before '%c'\n", save);
						c = save;
					}
					break;
				}
			}

			currentval = c;
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
			}else
				curtok = token_plus;
			break;
		case '-':
			if(peeknextchar() == '-'){
				nextchar();
				curtok = token_decrement;
			}else
				curtok = token_minus;
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
			}else
				curtok = token_lt;
			break;

		case '>':
			if(peeknextchar() == '='){
				nextchar();
				curtok = token_ge;
			}else
				curtok = token_gt;
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


		default:
			curtok = token_unknown;
	}
}
