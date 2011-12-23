#ifndef PREPROC_H
#define PREPROC_H

struct macro
{
	char *spel;
	char *replace;
};

void preprocess(void);

#endif
