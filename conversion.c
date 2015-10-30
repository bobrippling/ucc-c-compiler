main()
{
	float f = 1.0; // CHECK: /warning:.*conversion.*floating-point precision: 'double' to 'float'/
	enum E { A, B } e = 1; // CHECK: /warning:


turns f-p into int
changes signedness
truncates int
truncates fp
