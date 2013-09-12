#ifndef MAIN_H
#define MAIN_H

void dirname_push(char *d);
char *dirname_pop(void);

extern char **cd_stack;

extern char cpp_time[16], cpp_date[16], cpp_timestamp[64];

extern int option_line_info;

extern char *current_fname;
extern int no_output;

extern struct loc loc_tok;
#define current_line loc_tok.line

#define CPP_X(f, ...)      \
	do{                      \
		current_line--;        \
		preproc_backtrace();   \
		f(NULL, __VA_ARGS__);  \
		current_line++;        \
	}while(0)

#define CPP_WARN(...) CPP_X(WARN_AT, __VA_ARGS__)
#define CPP_DIE(... ) CPP_X(DIE_AT,  __VA_ARGS__)

void debug_push_line(char *);
void debug_pop_line(void);

#endif
