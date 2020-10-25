// RUN: %check %s
f()
{
	return 0;
}

f(); // CHECK: /warning: declaration of "f" after definition is ignored/
