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
} macro;

macro *macro_add(     const char *nam, const char *val);
macro *macro_add_func(const char *nam, const char *val, char **args, int variadic);

macro *macro_find(const char *sp);
void   macro_add_dir(char *d);
void   macro_remove(const char *nam);

void filter_macro(char **line);
void macro_finish(void);

#define TODO() do{die("%s: TODO", __func__); exit(1);}while(0)

enum
{
	DEBUG_NORM = 0,
	DEBUG_VERB = 1,
};
extern int debug;
#define DEBUG(level, ...) do{ if(level < debug) fprintf(stderr, ">>> " __VA_ARGS__); }while(0)

extern char **lib_dirs;

#endif
