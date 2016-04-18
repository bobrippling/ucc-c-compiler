// RUN: %check %s

f(int fn(void))
{
	((__attribute((warn_unused_result)) int (*)(int))fn)(1); // CHECK: /warning: unused expr/
}
