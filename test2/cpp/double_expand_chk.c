// RUN: %ucc -P -E %s | %output_check -w '__attribute__((unused)) int i;' '__attribute__((unused)) int32_t __j_1;' 'f(__attribute__((unused)))int32_t.__j_1;'

/* this tests where we resume after expanding a macro */

#define  nused BAD BAD BAD
#define unused __attribute__((unused))

unused int i;

// ----

#define int int32_t
#define i __j_1

unused int i;

f(unused)int.i;
