// -emit=print to avoid code gen
// RUN: %ucc -emit=print -fsyntax-only %s

#define CHK(exp, ty)                \
	_Static_assert(                   \
			__builtin_types_compatible_p( \
				__typeof(exp),                \
				ty),                        \
				"?")

void f(
	char char_,
	short short_,
	int int_,
	long long_,
	long long longlong_,
	float float_,
	double double_,
	long double longdouble_
)
{
	CHK(char_ + char_, int);
	CHK(char_ + short_, int);
	CHK(char_ + int_, int);
	CHK(char_ + long_, long);
	CHK(char_ + longlong_, long long);
	CHK(char_ + float_, float);
	CHK(char_ + double_, double);
	CHK(char_ + longdouble_, long double);

	CHK(short_ + char_, int);
	CHK(short_ + short_, int);
	CHK(short_ + int_, int);
	CHK(short_ + long_, long);
	CHK(short_ + longlong_, long long);
	CHK(short_ + float_, float);
	CHK(short_ + double_, double);
	CHK(short_ + longdouble_, long double);

	CHK(int_ + char_, int);
	CHK(int_ + short_, int);
	CHK(int_ + int_, int);
	CHK(int_ + long_, long);
	CHK(int_ + longlong_, long long);
	CHK(int_ + float_, float);
	CHK(int_ + double_, double);
	CHK(int_ + longdouble_, long double);

	CHK(long_ + char_, long);
	CHK(long_ + short_, long);
	CHK(long_ + int_, long);
	CHK(long_ + long_, long);
	CHK(long_ + longlong_, long long);
	CHK(long_ + float_, float);
	CHK(long_ + double_, double);
	CHK(long_ + longdouble_, long double);

	CHK(longlong_ + char_, long long);
	CHK(longlong_ + short_, long long);
	CHK(longlong_ + int_, long long);
	CHK(longlong_ + long_, long long);
	CHK(longlong_ + longlong_, long long);
	CHK(longlong_ + float_, float);
	CHK(longlong_ + double_, double);
	CHK(longlong_ + longdouble_, long double);

	CHK(float_ + char_, float);
	CHK(float_ + short_, float);
	CHK(float_ + int_, float);
	CHK(float_ + long_, float);
	CHK(float_ + longlong_, float);
	CHK(float_ + float_, float);
	CHK(float_ + double_, double);
	CHK(float_ + longdouble_, long double);

	CHK(double_ + char_, double);
	CHK(double_ + short_, double);
	CHK(double_ + int_, double);
	CHK(double_ + long_, double);
	CHK(double_ + longlong_, double);
	CHK(double_ + float_, double);
	CHK(double_ + double_, double);
	CHK(double_ + longdouble_, long double);

	CHK(longdouble_ + char_, long double);
	CHK(longdouble_ + short_, long double);
	CHK(longdouble_ + int_, long double);
	CHK(longdouble_ + long_, long double);
	CHK(longdouble_ + longlong_, long double);
	CHK(longdouble_ + float_, long double);
	CHK(longdouble_ + double_, long double);
	CHK(longdouble_ + longdouble_, long double);
}
