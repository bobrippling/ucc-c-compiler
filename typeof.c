// https://github.com/uecker/unqual/blob/main/unqual.c

/* Martin Uecker.
 *
 * macro that removes qualifiers from a type
 * */

#define typeof(x) __typeof__(x)

#define nil(T) ((typeof(T)){ 0 })
#define has_type_p(T, x) _Generic(nil(typeof(x)*), typeof(T)*: 1, default: 0)

// seems to work on both clang and gcc
#define lvalue_convert(x) ((void)0, x)
#define is_function_or_array_p(T) 				\
	(   (!has_type_p(lvalue_convert(T), T)) 		\
	 && (!has_type_p(const typeof(lvalue_convert(T)), T)))


#define choose_expr(x, a, b) _Generic(1?((void*)(x)):((int*)0), int*: (b), void*: (a))

#define pointer(x) typeof(typeof(x)*)
#define choose_type(x, a, b) typeof(*choose_expr(x, nil(pointer(a)), nil(pointer(b))))

#define pointer_to(x) choose_expr(is_function_or_array_p(x), (x), &(x))
#define base_type(x) (*pointer_to(x))

#define array(T, N) typeof(typeof(T)[N])
#define array_length2(T, E) (sizeof(T) / sizeof(E))

#define array_recreate(T, B) array(B, array_length2(T, B))

#define is_function_p(T) (is_function_or_array_p(T) && has_type_p(T, base_type(T)))
#define is_array_p(T) (is_function_or_array_p(T) && !has_type_p(T, base_type(T)))

#define compose(x, y) choose_type(is_array_p(x), array_recreate(x, y), y)

#ifdef __clang__
#define remove_qual(x) (((void)0, x))
#else
// GCC does not do lvalue conversion for comma
// pedantic complains about the cast
#define remove_qual(x) ((typeof(x))(x))
#endif

// may need recursion for multi-dim. arrays
#define unqual0(x) compose(x, remove_qual(base_type(x)))

#define unqual(x) choose_type(is_function_p(x), typeof(x), unqual0(choose_expr(!is_function_p(x), x, nil(int))))

#define unqual0_1(x) compose(x, unqual(base_type(x)))
#define unqual2(x) choose_type(is_function_p(x), typeof(x), unqual0_1(choose_expr(!is_function_p(x), x, nil(int))))

#define unqual0_2(x) compose(x, unqual2(base_type(x)))
#define unqual3(x) choose_type(is_function_p(x), typeof(x), unqual0_2(choose_expr(!is_function_p(x), x, nil(int))))




struct s { int x; };
extern struct s s;
extern int i;
extern int a[3];
extern int b[3][3];
extern int c[3][3][3];
extern int f(int x);

_Static_assert(!is_function_or_array_p(i), "");
_Static_assert(!is_function_or_array_p(s), "");
_Static_assert(is_function_or_array_p(a), "");
_Static_assert(is_function_or_array_p(f), "");

_Static_assert(!is_array_p(i), "");
_Static_assert(!is_array_p(s), "");
_Static_assert(is_array_p(a), "");
_Static_assert(!is_array_p(f), "");

_Static_assert(!is_function_p(i), "");
_Static_assert(!is_function_p(s), "");
_Static_assert(!is_function_p(a), "");
_Static_assert(is_function_p(f), "");

#define qual const

extern qual struct s sc;
extern qual int ic;
extern qual int ac[3];
extern qual int bc[3][3];

_Static_assert(!is_function_or_array_p(ic), "");
_Static_assert(!is_function_or_array_p(sc), "");
_Static_assert(is_function_or_array_p(ac), "");

_Static_assert(!is_array_p(ic), "");
_Static_assert(!is_array_p(sc), "");
_Static_assert(is_array_p(ac), "");


extern unqual(i) i;
extern unqual(s) s;
extern unqual(a) a;
// extern unqual2(b) b;
// extern unqual3(c) c;
extern unqual(f) f;

extern unqual(ac) a;
extern unqual(ic) i;
extern unqual(sc) s;
// extern unqual2(bc) b;
