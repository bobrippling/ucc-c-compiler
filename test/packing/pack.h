struct A
{
	// position indicates packing
	int a, b;
	char c;
	long l;
	short s1, s2, s3, s4;
	short s5; /**/ int i2;
};

void f(struct A *);
