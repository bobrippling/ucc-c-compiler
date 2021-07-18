// RUN: %check -e %s

#define __header(x) #x
#define _header(x) __header(dir/prefix ## x.h)
#define header(x) _header(x)

#define MAC 5

#include header(MAC) // CHECK: error: can't find include file "dir/prefix5.h"
