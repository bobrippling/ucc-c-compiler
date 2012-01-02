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
		TOKEN_OTHER
	} tok;
	char *w;
	int had_whitespace;
} token;


typedef struct
{
	char *nam, *val;
	char **args;
} macro;

void macro_add_dir(char *d);
void macro_add(const char *nam, const char *val);
macro *macro_find(const char *sp);
void macro_remove(const char *nam);

void filter_macro(char **line);
void handle_macro(char *line);
void macro_finish(void);


#define TODO() do{fprintf(stderr, "%s: TODO\n", __func__); exit(1);}while(0)
extern int debug;
enum
{
	DEBUG_NORM = 0,
	DEBUG_VERB = 1,
};
#define DEBUG(level, ...) if(level < debug) fprintf(stderr, ">>> " __VA_ARGS__)

#endif
