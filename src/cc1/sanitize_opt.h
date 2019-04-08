#ifndef SANITIZE_OPT
#define SANITIZE_OPT

#include "sanitize_opts.h"

enum san_opts
{
#define X(name, value, arg, desc) name = value,
	SANITIZE_OPTS
#undef X
};

extern enum san_opts cc1_sanitize;
extern char *cc1_sanitize_handler_fn;

void sanitize_opt_add(const char *argv0, const char *san);
void sanitize_opt_set_error(const char *argv0, const char *handler);
void sanitize_opt_off(void);

#endif
