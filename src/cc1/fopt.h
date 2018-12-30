#ifndef FOPT_H
#define FOPT_H

struct cc1_fopt
{
#define X(flag, name) unsigned char name;
#define INVERT(flag, name)
#define ALIAS(flag, name)
#define EXCLUSIVE(flag, name, excl) X(flag, name)
#define ALIAS_EXCLUSIVE(flag, name, excl)

#include "fopts.h"

#undef X
#undef INVERT
#undef ALIAS
#undef EXCLUSIVE
#undef ALIAS_EXCLUSIVE
};

void fopt_default(struct cc1_fopt *);

int fopt_on(struct cc1_fopt *, const char *argument, int invert);

#define FOPT_PIC(fopt) ((fopt)->pic || (fopt)->pie)

#endif
