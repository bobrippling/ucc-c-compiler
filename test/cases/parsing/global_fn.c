// RUN: %check --only %s

void f()
{
	void g() __attribute__((weak));
	// 'weak' is only allowed on global symbols - this test passing ensures 'g' is treated as a global

	g();
}
