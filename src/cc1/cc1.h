#ifndef CC1_H
#define CC1_H

#include "../util/std.h"

#include "warn.h"
#include "visibility.h"

enum mopt
{
	MOPT_32            = 1 << 0,
	MOPT_STACK_REALIGN = 1 << 1,
	MOPT_ALIGN_IS_POW2   = 1 << 2,
};
#define IS_32_BIT() (!!(mopt_mode & MOPT_32))

enum cc1_backend
{
	BACKEND_ASM,
	BACKEND_DUMP,
	BACKEND_STYLE,
};

enum san_opts
{
	CC1_UBSAN = 1 << 0
};

extern struct cc1_fopt cc1_fopt;
extern enum mopt mopt_mode;
extern enum cc1_backend cc1_backend;
extern enum san_opts cc1_sanitize;
extern char *cc1_sanitize_handler_fn;
extern enum visibility cc1_visibility_default;

extern enum c_std cc1_std;
#define C99_LONGLONG() \
	if(cc1_std < STD_C99) \
		cc1_warn_at(NULL, long_long, "long long is a C99 feature")

extern int cc1_error_limit;

extern int cc1_mstack_align; /* 2^n */
extern int cc1_gdebug; /* -g */

extern char *cc1_first_fname;

#endif
