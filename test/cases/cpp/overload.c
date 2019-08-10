// RUN: %ucc -E %s -P | %stdoutcheck %s
#define CAT(A, B) CAT2(A, B)
#define CAT2(A, B) A ## B

#define COUNT_PARMS2(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _, ...) _
#define COUNT_PARMS(...)\
	COUNT_PARMS2(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)

#define cpp_overload(...)\
	CAT(cpp_overload, COUNT_PARMS(__VA_ARGS__))(__VA_ARGS__)

cpp_overload(1);
cpp_overload(1, 2);
cpp_overload(1, 2, 3);

// STDOUT: cpp_overload1(1);
// STDOUT-NEXT: cpp_overload2(1, 2);
// STDOUT-NEXT: cpp_overload3(1, 2, 3);
