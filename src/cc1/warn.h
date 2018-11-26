#ifndef CC1_WARN_H
#define CC1_WARN_H

#include "../util/dynmap.h"

enum warning_fatality
{
	W_OFF = 0,
	W_WARN = 1,
	W_ERROR = 2, /* set by -Werror */

	W_NO_ERROR = 3
	/* set by -Wno-error=xyz
	 * not reset to W_WARN since -Werror/-Wno-error will alter this */
};

struct cc1_warning
{
#define X(flag, name) \
	unsigned char name;

#define ALIAS(flag, name)
#define GROUP(flag, ...)

#include "warnings.def"

#undef X
#undef ALIAS
#undef GROUP
};

extern struct cc1_warning cc1_warning;

/* returns 1 if a warning was emitted */
int cc1_warn_at_w(
		const struct where *where,
		const unsigned char *pwarn,
		const char *fmt, ...)
	ucc_printflike(3, 4);

#define cc1_warn_at(loc, warn, ...) \
		cc1_warn_at_w(loc, &cc1_warning.warn, __VA_ARGS__)

void warning_init(void);

void warnings_set(enum warning_fatality to);

void warning_pedantic(enum warning_fatality set);

void warning_on(
		const char *warn, enum warning_fatality to,
		int *const werror, dynmap *unknowns);

void warnings_upgrade(void);

int warnings_check_unknown(dynmap *unknown_warnings);

#endif
