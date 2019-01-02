void v(void *p)
{
}

void w(void *);

void x()
{
}

void y(void)
{
}

void z(int a, int b)
{
}

int main()
{
	//v();
	v(1);
	//v(1, 2);

	//w();
	w(1);
	//w(1, 2);

	y();
	//y(1);

	x();
	x(1);
	x(1, 2);

	//z();
	//z(1);
	z(1, 2);
	//z(1, 2, 3);
}

w(){}
