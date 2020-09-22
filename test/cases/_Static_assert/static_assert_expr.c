// RUN:   %ucc -DEXPR=1 -fsyntax-only -std=c99 %s
// RUN:   %ucc -DEXPR=1 -fsyntax-only -std=c11 %s
// RUN: ! %ucc -DEXPR=0 -fsyntax-only -std=c99 %s
// RUN: ! %ucc -DEXPR=0 -fsyntax-only -std=c11 %s

# if __STDC_VERSION__ >= 201112L
#  define expr_assert(expr)  ( \
        0 * (int)sizeof( \
          struct { \
            _Static_assert((expr), ""); \
            char nonempty; \
          } \
        ) \
)
# else
#  define expr_assert(expr)  ( \
        0 * (int)sizeof( \
          struct { \
            int  : -!(expr); \
            char nonempty; \
          } \
        ) \
)
# endif

int main()
{
	int a = (expr_assert(EXPR), 5);
}
