_Atomic struct {
	char buf[4096];
} a, a2;

_Atomic(struct {
	char buf[4096];
}) b;

f()
{
	//a.buf[3] = 3;

	a = a2;
}

// ----------------------

// https://gcc.gnu.org/pipermail/gcc/2020-June/232976.html

// cc atomic.c -latomic -g -fpie -Wl,-z,now

typedef struct { float re; float im; } Real;

Real bar2(Real *x)
{
	Real r;
	__atomic_load(x, &r, __ATOMIC_SEQ_CST);
	return r;
}

// ----------------------

int printf(const char *, ...);
int putchar(int);
#define countof(a) sizeof(a)/sizeof((a)[0])

typedef _Atomic struct N {
	long long a[10];
} N;

N n;

int main()
{
	// can't __atomic_load_n() something that's not a machine-word (or smaller)
	//struct N x = __atomic_load_n(&n, __ATOMIC_RELAXED);

	struct N x;
	__atomic_load(&n, &x, __ATOMIC_RELAXED);

	const char *sep = "";
	for(int i = 0; i < countof(x.a); i++){
		printf("%s%lld", sep, x.a[i]);
		sep = ", ";
	}
	putchar('\n');
}

//#define __ATOMIC_ACQUIRE 2
//#define __GCC_ATOMIC_CHAR_LOCK_FREE 2
//#define __GCC_ATOMIC_CHAR32_T_LOCK_FREE 2
//#define __SIG_ATOMIC_TYPE__ int
//#define __GCC_ATOMIC_BOOL_LOCK_FREE 2
//#define __GCC_ATOMIC_POINTER_LOCK_FREE 2
//#define __ATOMIC_HLE_RELEASE 131072
//#define __ATOMIC_HLE_ACQUIRE 65536
//#define __GCC_ATOMIC_INT_LOCK_FREE 2
//#define __SIG_ATOMIC_MAX__ 0x7fffffff
//#define __GCC_ATOMIC_WCHAR_T_LOCK_FREE 2
//#define __GCC_ATOMIC_LONG_LOCK_FREE 2
//#define __SIG_ATOMIC_WIDTH__ 32
//#define __SIG_ATOMIC_MIN__ (-__SIG_ATOMIC_MAX__ - 1)
//#define __GCC_ATOMIC_TEST_AND_SET_TRUEVAL 1
//#define __GCC_ATOMIC_CHAR16_T_LOCK_FREE 2
//#define __ATOMIC_RELAXED 0
//#define __ATOMIC_CONSUME 1
//#define __ATOMIC_SEQ_CST 5
//#define __GCC_ATOMIC_LLONG_LOCK_FREE 2
//#define __GCC_ATOMIC_SHORT_LOCK_FREE 2
//#define __ATOMIC_ACQ_REL 4
//#define __ATOMIC_RELEASE 3
