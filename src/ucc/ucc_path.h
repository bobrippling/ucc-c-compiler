#ifndef UCC_PATH_H
#define UCC_PATH_H

struct cmdpath
{
	const char *path;
	enum cmdpath_type
	{
		RELATIVE_TO_UCC, /* default for cc1 and cpp */
		USE_PATH, /* default for 'as' and 'ld' */
		FROM_Bprefix /* used if -B is given */
	} type;
};

typedef int cmdpath_exec_fn(const char *, char *const[]);

void cmdpath_initrelative(
		struct cmdpath *,
		const char *bprefix,
		const char *ucc_relative);

const char *cmdpath_type(enum cmdpath_type);
char *cmdpath_resolve(const struct cmdpath *, cmdpath_exec_fn **);

/* free()s argument */
char *path_prepend_relative(char *);

char *dirname_ucc(void);

#endif
