union fp
{
	float f;
	struct
	{
		unsigned fract : 23, exponent : 8, sign : 1;
	} bits;
};

f(union fp *u);
