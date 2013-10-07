#ifndef LABEL_H
#define LABEL_H

struct label
{
	where *pw;
	char *spel;
	unsigned uses;
	int complete;
};
typedef struct label label;

label *label_new(where *, char *, int complete);

#endif
