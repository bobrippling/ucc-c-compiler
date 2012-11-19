#define cpp_default1(A) cpp_default2(A, "default string")

void cpp_default2(int x, const char *s)
{
	printf("Got %d %s\n", x, s);
}

#define cpp_default(...)\
	CAT(cpp_default, COUNT_PARMS(__VA_ARGS__))(__VA_ARGS__)
