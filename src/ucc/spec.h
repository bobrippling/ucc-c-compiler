#ifndef SPEC_H
#define SPEC_H

struct specvars
{
	int static_, shared, pie;
	int debug;
	int stdlibinc, builtininc;
	int startfiles, defaultlibs;
	const char *output;
};

struct specopts
{
	char **initflags;

	char *as;
	char **asflags;

	char *ld;
	char **ldflags_pre_user;
	char **ldflags_post_user;

	char *post_link;
};

struct specerr
{
	const char *errstr;
	unsigned errline;
};

void spec_parse(
		struct specopts *,
		const struct specvars *,
		FILE *,
		struct specerr *);

void spec_default(struct specopts *);

void spec_dump(struct specopts *opts, FILE *f);

#endif
