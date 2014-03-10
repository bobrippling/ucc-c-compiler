#ifndef MACRO_H
#define MACRO_H

#define DEFINED_STR "defined"
/*#define EVAL_DEBUG*/

typedef struct
{
	where where;
	char *nam, *val;
	enum { MACRO, FUNC, VARIADIC } type;
	char **args;
	int blue; /* being evaluated? */
	int use_cnt; /* track usage for double-eval */
	int use_dump; /* for -dS */
} macro;

macro *macro_add(     const char *nam, const char *val);
macro *macro_add_func(const char *nam, const char *val,
		char **args, int variadic);

macro *macro_find(const char *sp);
int    macro_remove(const char *nam);
void   macros_dump(void);
void   macros_stats(void);
void macros_warn_unused(void);

extern macro **macros;

void macro_use(macro *m, int adj);

#endif
