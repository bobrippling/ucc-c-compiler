// RUN: %ucc -fsyntax-only %s

#define IF_TY_EQ(ty, a, b)          \
	_Static_assert(                    \
			__builtin_types_compatible_p(  \
				ty, typeof(0 ? (a)0 : (b)0)),  \
			"")

#define IF_EXPR_EQ(ty, a, b) IF_TY_EQ(ty, typeof(a), typeof(b))

typedef struct A A;
typedef struct B B;

// warning
IF_TY_EQ(void *, A *, B *);

IF_TY_EQ(void, void, B *);

IF_TY_EQ(void, A *, void);

IF_EXPR_EQ(A *, A *, 0);

// warning
IF_EXPR_EQ(A *, A *, 1);

IF_TY_EQ(A *, A *, void *);

// clang failure
IF_TY_EQ(volatile const A *, volatile A *, const A *);

/* volatile void * is not the null pointer constant */
IF_TY_EQ(const volatile void *, A const *, volatile void *); // FIXME

/* const void * is not the null pointer constant */
IF_TY_EQ(const void *, const A *, const void *); // FIXME

/* -------- */
typedef const void *c_vp;
typedef void *vp;
typedef const int *c_ip;
typedef volatile int *v_ip;
typedef int *ip;
typedef const char *c_cp;

IF_TY_EQ(const void *, c_vp, c_ip); // FIXME
IF_EXPR_EQ(volatile int *, (v_ip)0, 0); // clang failure
IF_TY_EQ(const volatile int *, c_ip, v_ip); // clang failure // FIXME
IF_TY_EQ(const void *, vp, c_cp); // gcc+clang failure // FIXME
IF_TY_EQ(const int *, ip, c_ip); // clang failure // FIXME
IF_TY_EQ(void *, vp, ip); // gcc failure // FIXME
