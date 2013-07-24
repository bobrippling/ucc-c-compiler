// RUN: [ `%ucc -S -o- %s | grep 'cvtsi2ss ' | wc -l` -eq 2 ]
// RUN: [ `%ucc -S -o- %s | grep 'cvtsi2ssq ' | wc -l` -eq 1 ]
// RUN: %ucc -c %s

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
