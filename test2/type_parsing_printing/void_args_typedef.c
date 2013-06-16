// RUN: %ucc -c %s
// RUN: ! %ucc -DA -c %s
// RUN: ! %ucc -DB -c %s
// RUN: ! %ucc -DC -c %s

typedef void v;
f(v);

// should error:
#ifdef A
g(v i);
#endif
#ifdef B
g(v, int);
#endif
#ifdef C
g(v i, int j);
#endif
