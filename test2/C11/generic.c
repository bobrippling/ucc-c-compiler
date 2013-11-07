// RUN: echo TODO; false
#include <stdio.h>

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

#define print(X, Y)                   \
_Generic( typeof2(X, Y),               \
    types_ii: printf("int, int\n"),     \
    types_id: printf("int, double\n"),   \
    types_dd: printf("double, double\n"), \
    types_di: printf("double, int\n"),     \
    default: printf("Something Else\n")     )

int main()
{
    print(1, 2);
    print(1, 2.0);
    print(1.0, 2.0);
    print(1.0, 2);
}
