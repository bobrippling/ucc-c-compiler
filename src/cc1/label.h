#ifndef LABEL_H
#define LABEL_H

struct label
{
	where *pw;
	char *spel, *mangled;
	unsigned uses;
	unsigned complete : 1, unused : 1;
};
typedef struct label label;

label *label_new(where *, char *fn, char *id, int complete);

#endif
