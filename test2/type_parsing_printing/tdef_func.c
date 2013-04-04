// RUN: %ucc -c %s
typedef struct { int i, j; } TWO_INTS, *P_TWO_INTS;

TWO_INTS *pf(void) /* this decays... ? */
{
}

P_TWO_INTS f(void)
{
}
