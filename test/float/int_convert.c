// RUN: [ `%ucc -S -o- %s | grep 'cvtsi2ssl ' | wc -l` -eq 2 ]
// RUN: [ `%ucc -S -o- %s | grep 'cvtsi2ssq ' | wc -l` -eq 1 ]
// RUN: %ucc -c %s

_Static_assert(
		__builtin_types_compatible_p(
			__typeof(5LL + 1.0f),
			float),
		"long long + float should be float");

float l_to_float(long long l)
{
	return l;
}

float i_to_float(int i)
{
	return i;
}

float s_to_float(int i)
{
	short s = i;
	return s;
}
