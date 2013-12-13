// RUN: %check %s

f(int i){}

main()
{
	f(1    << 35); // CHECK: /warning: shift count >= /
	f(1U   << 35); // CHECK: /warning: shift count >= /
	f(1L   << 35); // CHECK: !/warnng: shift count/
	f(1LU  << 35); // CHECK: !/warnng: shift count/
	f(1ULL << 35); // CHECK: !/warnng: shift count/
}
