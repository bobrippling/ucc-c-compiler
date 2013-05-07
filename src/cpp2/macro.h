#ifndef MACRO_H
#define MACRO_H

typedef struct
{
	enum tok
	{
		TOKEN_WORD,
		TOKEN_OPEN_PAREN,
		TOKEN_CLOSE_PAREN,
		TOKEN_COMMA,
		TOKEN_ELIPSIS,
		TOKEN_OTHER
	} tok;
	char *w;
	int had_whitespace;
} token;


typedef struct
{
	char *nam, *val;
	enum { MACRO, FUNC, VARIADIC } type;
	char **args;
	int blue; /* in eval context? */
} macro;

macro *macro_add(     const char *nam, const char *val);
macro *macro_add_func(const char *nam, const char *val, char **args, int variadic);

macro *macro_find(const char *sp);
void   macro_add_dir(char *d);
void   macro_remove(const char *nam);

char *filter_macro(char *line);
void macro_finish(void);

enum
{
	DEBUG_NORM = 0,
	DEBUG_VERB = 1,
};
extern int option_debug;
#define DEBUG(level, ...) do{ if(level < option_debug) fprintf(stderr, ">>> " __VA_ARGS__); }while(0)

extern char **lib_dirs;

#endif
