typedef _Bool bool;

double sqrt(double);
int printf(const char *, ...);

#  define va_start __builtin_va_start
#  define va_arg __builtin_va_arg
#  define va_end __builtin_va_end
typedef __builtin_va_list va_list;


double sub();

double one()
{
	return 1;
}

double zero()
{
	return sub(one(), one());
}

double nan()
{
	return one() / zero();
}

double add(double a, ...)
{
	va_list l;
	va_start(l, a);

	double x;
	while((x = va_arg(l, double)) != nan()){
		a += x;
	}

	va_end(l);

	return a;
}

double sub(double a, double b)
{
	return a - b;
}

double mul(double a, double b)
{
	return a * b;
}

double div(double a, double b)
{
	return a / b;
}

bool equal(double a, double b)
{
	return a == b;
}

double phi()
{
	return div(add(one(), sqrt(add(one(), one(), one(), one(), one(), nan())), nan()),
			add(one(), one(), nan()));
}

double psi()
{
	return sub(one(), phi());
}

double exp(double a, double b)
{
	if(equal(b, zero())){
		return one();
	}else{
		return mul(a, exp(a, sub(b, one())));
	}
}

int fib(int n)
{
	return div(sub(exp(phi(), n), exp(psi(), n)), sub(phi(), psi()));
}

int main()
{
	for(int i = 0; i < 10; i++)
		printf("fib(%d) = %d\n", i, fib(i));
	return 0;
}

#ifdef MORE
double sqrt(double);

main()
{
	typedef long long T;
	for(T i = 0; i < sqrt(i); i++)
		;
}
#endif
