#ifndef FOPT_H
#define FOPT_H

struct cc1_fopt
{
#define X(flag, name) unsigned char name;
#define INVERT(flag, name)
#define ALIAS(flag, name)

#include "fopts.h"

#undef X
#undef INVERT
#undef ALIAS
};

void fopt_default(struct cc1_fopt *);

#endif
