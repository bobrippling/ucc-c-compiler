#ifndef CC1_WARN_H
#define CC1_WARN_H

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

#endif
