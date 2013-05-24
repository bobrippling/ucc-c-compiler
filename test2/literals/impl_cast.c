// RUN: %check %s
f()
{
	return 0xfffffffffffffff; // CHECK: /warning: implicit cast changes value/
}
