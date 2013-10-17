// RUN: %ucc -c %s
// RUN: %ucc -DNAMED -c %s; [ $? -ne 0 ]
// RUN: %ucc -DFIRST -c %s; [ $? -ne 0 ]
// RUN: %ucc -DFIRST_NAMED -c %s; [ $? -ne 0 ]

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
