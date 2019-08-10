// RUN: %ucc -o %t %s -fsanitize=float-divide-by-zero -fsanitize=integer-divide-by-zero -fsanitize-error=call=san_fail
// RUN: %ocheck 0 %t normal
// RUN: %ocheck 2 %t div0
// RUN: %ocheck 2 %t intmin
// (disabled) RUN %ocheck 2 %t floatdiv0

#define INT_MIN (-__INT_MAX__ - 1)

void san_fail(void)
{
	_Noreturn void exit(int);
	exit(2);
}

int f(int a, int b)
{
	return a/b;
}

/*
float g(float a, float b)
{
	return a/b;
}
*/

int strcmp(const char *, const char *);

int main(int argc, const char **argv)
{
	if(!strcmp(argv[1], "normal"))
		return f(INT_MIN,-2);
	if(!strcmp(argv[1], "div0"))
		return f(1, 0);
	if(!strcmp(argv[1], "intmin"))
		return f(INT_MIN,-1);
	/*
	if(!strcmp(argv[1], "floatdiv0"))
		return g(3, 0);
	*/

	return 5;
}
