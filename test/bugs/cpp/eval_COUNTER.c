#define PASTE(A, B) A##B

#define MIN_IMPL(A, B, L) \
	({ \
		__typeof(A) PASTE(a, L) = (A); \
		__typeof(B) PASTE(b, L) = (B); \
		(PASTE(a, L) < PASTE(b, L)) ? PASTE(a, L) : PASTE(b, L); \
	})

#define MIN(A, B) MIN_IMPL(A, B, __COUNTER__)

MIN(x, y)
