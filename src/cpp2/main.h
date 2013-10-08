#ifndef MAIN_H
#define MAIN_H

void dirname_push(char *d);
char *dirname_pop(void);

extern char **cd_stack;

extern char cpp_time[16], cpp_date[16], cpp_timestamp[64];

extern int option_line_info;
extern int option_trigraphs, option_digraphs;

extern char *current_fname;
extern int no_output;

extern struct loc loc_tok;
#define current_line loc_tok.line

#define CPP_X(wm, f, ...)      \
  do{                          \
    if(wm == 0 || wm & wmode){ \
      current_line--;          \
      preproc_backtrace();     \
      f(NULL, __VA_ARGS__);    \
      current_line++;          \
    }                          \
  }while(0)

#define CPP_WARN(wm, ...) CPP_X(wm, warn_at, __VA_ARGS__)
#define CPP_DIE(... )     CPP_X(0,  die_at,  __VA_ARGS__)

void debug_push_line(char *);
void debug_pop_line(void);

extern enum wmode
{
	WTRADITIONAL = 1 << 0,
	WUNDEF       = 1 << 1, /* #if abc and #undef abc, where abc isn't defined */
	WUNUSED      = 1 << 2,
	WREDEF       = 1 << 3, /* #define a, #define a */
	WWHITESPACE  = 1 << 4, /* #define f(a)a */
	WTRAILING    = 1 << 5, /* #endif yo */
	WEMPTY_ARG   = 1 << 6, /* #define F(x), F() */
	WPASTE       = 1 << 7, /* "< ## >" */
	WUNCALLED_FN = 1 << 8, /* #define F(x), F */
	WFINALESCAPE = 1 << 9, /* backslash-esc at eof */
	WMULTICHAR   = 1 << 10, /* duh */
} wmode;

extern enum comment_strip
{
	STRIP_ALL, /* <empty> */
	STRIP_EXCEPT_DIRECTIVE, /* -C */
	STRIP_NONE /* -CC */
} strip_comments;

#endif
