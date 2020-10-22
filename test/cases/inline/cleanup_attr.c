// RUN: %check %s -fshow-inlined -fno-semantic-interposition
// RUN: %ocheck 10 %s -fno-semantic-interposition

__attribute((noinline))
void clean_int_ni(int *p)
{
	*p = 3;
}

__attribute((always_inline))
inline void clean_int_ai(int *p)
{
	*p = 3;
}

main()
{
	__attribute((cleanup(clean_int_ai))) int i = 5; // CHECK: note: function inlined
	__attribute((cleanup(clean_int_ni))) int j = 5; // CHECK: !/inline/

	return i + j;
}
