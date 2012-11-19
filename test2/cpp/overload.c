void cpp_overload1(int p1)
{
	printf("CPP One param: %d\n", p1);
}

void cpp_overload2(double *p1, const char *p2)
{
	printf("CPP Two params: %p (%f) %s\n", p1, *p1, p2);
}

void cpp_overload3(int p1, int p2, int p3)
{
	printf("CPP Three params: %c %d %d\n", p1, p2, p3);
}

#define CAT(A, B) CAT2(A, B)
#define CAT2(A, B) A ## B

#define cpp_overload(...)\
	CAT(cpp_overload, COUNT_PARMS(__VA_ARGS__))(__VA_ARGS__)
