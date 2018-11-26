// RUN: %ucc -emit=print %s | grep -F "a 'typeof(typeof(size_t))'"

typedef long unsigned size_t;
__typeof(__typeof(size_t)) a;
