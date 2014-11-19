// RUN: %ucc -fsyntax-only %s

typedef struct
{
	long long ll;
	long double ld;
} max_align_t_1;

typedef char max_align_t_2 __attribute__((aligned(__BIGGEST_ALIGNMENT__)));

long long x;
struct
{
	long long x;
	__attribute((aligned(8))) char ch1;
	_Alignas(4) char ch2;
} y;

max_align_t_1 t1;
max_align_t_2 t2;

int _Alignas(max_align_t_1) yo;

_Static_assert(_Alignof(x) == 8, "");
_Static_assert(_Alignof(y.x) == 8, "");
_Static_assert(_Alignof(yo) == 16, "");
_Static_assert(_Alignof(max_align_t_1) == 16, "");
_Static_assert(_Alignof(max_align_t_2) == 16, "");
_Static_assert(_Alignof(y.ch1) == 8, "");
_Static_assert(_Alignof(y.ch2) == 4, "");

int parse = _Alignof(y).x;

typedef char max_align_t_3 __attribute__((aligned()));
typedef char max_align_t_4 __attribute__((aligned));
_Static_assert(_Alignof(max_align_t_2) == 16, "");
_Static_assert(_Alignof(max_align_t_3) == 16, "");
