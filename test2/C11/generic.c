// RUN: %ocheck 0 %s

// generic lib
typedef struct { int _; } types_dd;
typedef struct { int _; } types_di;
typedef struct { int _; } types_id;
typedef struct { int _; } types_ii;

typedef struct { types_dd Double; types_di Int; } types_d;
typedef struct { types_id Double; types_ii Int; } types_i;

typedef struct { types_d Double; types_i Int; } types_unit;

#define typeof0() (*((types_unit*)0))

#define typeof1(X)        \
_Generic( (X),             \
    int:    typeof0().Int,  \
    double: typeof0().Double )

#define typeof2(X, Y)      \
_Generic( (Y),              \
    int:    typeof1(X).Int,  \
    double: typeof1(X).Double )


// generic client
enum
{
	int_int,
	int_double,
	double_double,
	double_int,
	unknown
};

#define pair_type(X, Y)      \
_Generic( typeof2(X, Y),     \
    types_ii: int_int,       \
    types_id: int_double,    \
    types_dd: double_double, \
    types_di: double_int,    \
    default: unknown)

int main()
{
	if(pair_type(1, 2) != int_int)
		abort();

	if(pair_type(1, 2.0) != int_double)
		abort();

	if(pair_type(1.0, 2.0) != double_double)
		abort();

	if(pair_type(1.0, 2) != double_int)
		abort();

	return 0;
}
