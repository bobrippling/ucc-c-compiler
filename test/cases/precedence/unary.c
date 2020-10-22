// RUN: %check %s
f()
{
	return 1 << -5; // CHECK: !/warning:.*precedence/
}

