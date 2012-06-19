#ifndef __STDBOOL_H
#define __STDBOOL_H

#ifdef __GOT_SHORT_LONG
typedef _Bool bool;
#else
typedef int bool;
#endif

#define true  1
#define false 0

#endif
