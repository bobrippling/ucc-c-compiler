// RUN: %check --only %s

int f()
{
	return 0xfffffffffffffff; // CHECK: warning: implicit cast changes value from 1152921504606846975 to -1
}
