#ifndef PP_H
#define PP_H

struct pp
{
	const char *fname;
	int nline;
	FILE *in, *out;
};

enum proc_ret
{
	PROC_ELSE,
	PROC_ENDIF,
	PROC_EOF,
	PROC_ERR
};

void adddef(const char *n, const char *v);
void adddir(const char *d);
enum proc_ret preprocess(struct pp *, int verbose);

#endif
