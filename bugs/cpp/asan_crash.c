#define M_C2I(a, ...)       a ## __VA_ARGS__
#define M_C(a, ...)         M_C2I(a, __VA_ARGS__)
#define M_C3I(a, b, ...)    a ## b ## __VA_ARGS__
#define M_C3(a, b, ...)     M_C3I(a ,b, __VA_ARGS__)

#define M_RETI_ARG2(a, b, ...)  b
#define M_RET_ARG2(...)         M_RETI_ARG2(__VA_ARGS__)
#define M_RETI_ARG27(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,aa, ...)    aa
#define M_RET_ARG27(...)        M_RETI_ARG27(__VA_ARGS__)

#define M_TOBOOLI_0                 1, 0,
#define M_BOOL(x)                   M_RET_ARG2(M_C(M_TOBOOLI_, x), 1, useless)

#define M_IFI_0(true_macro, ...)    __VA_ARGS__
#define M_IFI_1(true_macro, ...)    true_macro
#define M_IF(c)                     M_C(M_IFI_, M_BOOL(c))

#define M_FLAT(...)                 __VA_ARGS__

#define M_INVI_0                    1
#define M_INVI_1                    0
#define M_INV(x)                    M_C(M_INVI_, x)

#define M_ANDI_00                   0
#define M_ANDI_01                   0
#define M_ANDI_10                   0
#define M_ANDI_11                   1
#define M_AND(x,y)                  M_C3(M_ANDI_, x, y)

#define M_ORI_00                    0
#define M_ORI_01                    1
#define M_ORI_10                    1
#define M_ORI_11                    1
#define M_OR(x,y)                   M_C3(M_ORI_, x, y)

#define M_COMMA_P(...)              M_RET_ARG27(__VA_ARGS__, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, useless)

#define M_EMPTYI_DETECT(...)        0, 1,
#define M_EMPTYI_P_C1(...)          M_COMMA_P(M_EMPTYI_DETECT __VA_ARGS__ () )
#define M_EMPTYI_P_C2(...)          M_COMMA_P(M_EMPTYI_DETECT __VA_ARGS__)
#define M_EMPTYI_P_C3(...)          M_COMMA_P(__VA_ARGS__ () )
#define M_EMPTY_P(...)              M_AND(M_EMPTYI_P_C1(__VA_ARGS__), M_INV(M_OR(M_OR(M_EMPTYI_P_C2(__VA_ARGS__), M_COMMA_P(__VA_ARGS__)),M_EMPTYI_P_C3(__VA_ARGS__))))

#define M_APPLY_FUNC2B(func, arg1, arg2)        \
  M_IF(M_EMPTY_P(arg2))(,func(arg1, arg2))
#define M_MAP2B_0(func, data, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z,...) \
  M_APPLY_FUNC2B(func, data, a) M_APPLY_FUNC2B(func, data, b)
M_APPLY_FUNC2B(func, data, c) \
  M_APPLY_FUNC2B(func, data, d) M_APPLY_FUNC2B(func, data, e)
M_APPLY_FUNC2B(func, data, f) \
  M_APPLY_FUNC2B(func, data, g) M_APPLY_FUNC2B(func, data, h)
M_APPLY_FUNC2B(func, data, i) \
  M_APPLY_FUNC2B(func, data, j) M_APPLY_FUNC2B(func, data, k)
M_APPLY_FUNC2B(func, data, l) \
  M_APPLY_FUNC2B(func, data, m) M_APPLY_FUNC2B(func, data, n)
M_APPLY_FUNC2B(func, data, o) \
  M_APPLY_FUNC2B(func, data, p) M_APPLY_FUNC2B(func, data, q)
M_APPLY_FUNC2B(func, data, r) \
  M_APPLY_FUNC2B(func, data, s) M_APPLY_FUNC2B(func, data, t)
M_APPLY_FUNC2B(func, data, u) \
  M_APPLY_FUNC2B(func, data, v) M_APPLY_FUNC2B(func, data, w)
M_APPLY_FUNC2B(func, data, x) \
  M_APPLY_FUNC2B(func, data, y) M_APPLY_FUNC2B(func, data, z)
#define M_MAP2B(f, ...) M_MAP2B_0(f, __VA_ARGS__, , , , , , , , , , , , , , , , , , , , , , , , , , , , , , , , , , , , , , , , , , , , , , , , , , , , , , )

#define M_INIT_INIT(a)           ,a,

#define M_GET_METHOD(method, method_default, ...)                       \
  M_RET_ARG2 (M_MAP2B(M_C, M_C3(M_, method, _), __VA_ARGS__), method_default,)

#define M_TEST_METHOD_P(method, oplist)                         \
  M_BOOL(M_GET_METHOD (method, 0, M_FLAT oplist))

#define TRUE 1
#define TEST1(n)                                \
  M_IF(n)(ok,nok)
#define TEST2(op)                               \
  M_TEST_METHOD_P(INIT, op)
#define TEST3(op)                               \
  M_IF(M_TEST_METHOD_P(INIT, op))(ok, nok)
#define KO(a) ((void)1)

TEST1(TRUE)          // ==> expect ok, get ok
// token is not a macro
TEST2((INIT(ok)))    // ==> expect 1, get 1
TEST3((INIT(ok)))    // ==> expect ok, get ok
// token is a macro, but should not be expanded
TEST2((INIT(KO)))    // ==> expect 1, get 1
TEST3((INIT(KO)))    // ==> expect ok
