#define __header(x) #x
#define _header(x) __header(yo/pre##x.h)
#define header(x) _header(x)

#define X 23

#include header(X)
