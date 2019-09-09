#ifndef CC1_H
#define CC1_H

#include "../util/std.h"

#include "warn.h"
#include "visibility.h"

enum mopt
{
	MOPT_STACK_REALIGN = 1 << 0,
	MOPT_ALIGN_IS_POW2   = 1 << 1,
	MOPT_FENTRY = 1 << 2,
};
#define IS_32_BIT() (platform_word_size() == 4)

enum stringop_strategy
{
	STRINGOP_STRATEGY_LIBCALL,
	STRINGOP_STRATEGY_LOOP,
	STRINGOP_STRATEGY_THRESHOLD,
};

enum cc1_backend
{
	BACKEND_ASM,
	BACKEND_DUMP,
	BACKEND_STYLE,
};

extern struct cc1_fopt cc1_fopt;
extern enum mopt mopt_mode;
extern enum cc1_backend cc1_backend;
extern enum visibility cc1_visibility_default;

extern enum stringop_strategy cc1_mstringop_strategy;
extern unsigned cc1_mstringop_threshold;

extern enum c_std cc1_std;
#define C99_LONGLONG() \
	if(cc1_std < STD_C99) \
		cc1_warn_at(NULL, long_long, "long long is a C99 feature")

extern int cc1_error_limit;

extern int cc1_mstack_align;
extern int cc1_profileg; /* -pg */

enum debug_level
{
	DEBUG_OFF,
	DEBUG_FULL,
	DEBUG_LINEONLY
};
extern enum debug_level cc1_gdebug; /* -g */
extern int cc1_gdebug_columninfo;

extern char *cc1_first_fname;

#endif
