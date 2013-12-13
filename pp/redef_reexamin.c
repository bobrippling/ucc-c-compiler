#define  x         3
#define  f(a)      f(x * (a))
#undef   x         2
#define  x         f
#define  g         z[0]
#define  z         g(~
#define  h         a(w)
#define  m(a)      0,1
#define  w         a
#define  t(a)      int
#define  p()       x
#define  q(x)      x ## y
#define  r(x,y)    # x
#define  str(x)

f(y+1) + f(f(z)) % t(t(g)(0) + t)(1);
g(x+(3,4)-w) | h 5) & m
(f)^m(m);
p() i[q()] = { q(1), r(2,3), r(4,), r(,5), r(,) };
char c[2][6] = { str(hello), str() };

// results in

f(2 * (y+1)) + f(2 * (f(2 * (z[0])))) % f(2 * (0)) + t(1);
f(2 * (2+(3,4)-0,1)) | f(2 * (~ 5)) & f(2 * (0,1))^m(0,1);
int i[] = { 1, 23, 4, 5, };
char c[2][6] = { "hello", "" };

