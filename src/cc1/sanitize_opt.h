#ifndef SANITIZE_OPT
#define SANITIZE_OPT

#include "sanitize_opts.h"
#include "enum_no_sanitize.h"

enum san_opts
{
#define X(name, value, disable, arg, desc) name = value,
	SANITIZE_OPTS
#undef X
};

extern char *cc1_sanitize_handler_fn;

void sanitize_opt_add(const char *argv0, const char *san);
void sanitize_opt_set_error(const char *argv0, const char *handler);
void sanitize_opt_off(void);

int sanitize_enabled(enum san_opts opt, enum no_sanitize disabled);

#endif
