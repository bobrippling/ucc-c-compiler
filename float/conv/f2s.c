extern int write();

void i2s_r(int i)
{
	int rem = i % 10;

	if((i /= 10))
		i2s_r(i);

	i = '0' + rem;
	write(1, &i, 1);
}

void i2s(int i)
{
	if(i < 0)
		write(1, "-", 1), i = -i;

	if(i == 0)
		write(1, "0", 1);
	else
		i2s_r(i);
}

void f2s(double f)
{
	int i = f;

	i2s(i);

	if(f < 0.0){
		f = -f;
		i = -i;
	}

	f -= i;

	write(1, ".", 1);

	// f is 0.something
	i = 0;

	// naive. *= 10.0 is lossy
	while(f > (int)f){
		f *= 10.0;
		i = i * 10 + (int)f;
		f -= (int)f;
	}

	i2s(i);
}
