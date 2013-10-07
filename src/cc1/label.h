#ifndef LABEL_H
#define LABEL_H

struct label
{
	where *pw;
	char *spel;
	int complete : 1;
};
typedef struct label label;

label *label_new(where *, char *, int complete);

#endif
