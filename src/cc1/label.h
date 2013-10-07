#ifndef LABEL_H
#define LABEL_H

struct label
{
	where *pw;
	char *spel;
	unsigned uses;
	unsigned complete : 1, unused : 1;
};
typedef struct label label;

label *label_new(where *, char *, int complete);

#endif
