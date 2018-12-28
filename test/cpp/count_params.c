#define COUNT_PARMS2(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _, ...) _
#define COUNT_PARMS(...) \
	COUNT_PARMS2(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)

#define f(...) COUNT_PARMS(__VA_ARGS__)

// RUN: %ucc -o %t %s
// RUN: %t

main()
{
	if(f(1, 2, 3) != 3)
		return 1;
	return 0;
}
