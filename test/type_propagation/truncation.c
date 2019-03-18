// RUN: %check %s -Wtruncation

main()
{
	char c = 0;
	short s = 0;
	int i = 0;
	long l = 0;
	long long ll = 0;
	float f = 0;
	double d = 0;

	c = 5; // CHECK: !/warn.*trunc/

	c = s; // CHECK: warning: possible truncation converting short to char
	s = c; // CHECK: !/warn.*trunc/

	s = l; // CHECK: warning: possible truncation converting long to short
	i = c; // CHECK: !/warn.*trunc/

	f = d; // CHECK: warning: possible truncation converting double to float
	d = f; // CHECK: !/warn.*trunc/
}
