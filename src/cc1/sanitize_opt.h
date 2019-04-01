#ifndef SANITIZE_OPT
#define SANITIZE_OPT

enum san_opts
{
	SAN_UBSAN = 1 << 0
};

extern enum san_opts cc1_sanitize;
extern char *cc1_sanitize_handler_fn;

void sanitize_opt_add(const char *argv0, const char *san);
void sanitize_opt_set_error(const char *argv0, const char *handler);

#endif
