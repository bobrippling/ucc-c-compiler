// RUN:   %ucc -c %s
// RUN: ! %ucc -c %s -DNAMED
// RUN: ! %ucc -c %s -DFIRST
// RUN: ! %ucc -c %s -DFIRST_NAMED

typedef void v;
f(v);

// should error:
#ifdef NAMED
g(v i);
#endif
#ifdef FIRST
g(v, int);
#endif
#ifdef FIRST_NAMED
g(v i, int j);
#endif
