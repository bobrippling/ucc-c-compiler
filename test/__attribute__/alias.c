// RUN: %ucc -S -o %t %s -target x86_64-linux
// RUN: grep 'f = f_impl' %t
// RUN: %check %s

int f_impl()
{
	printf("%s()\n", __func__);
	return 3;
}

extern __typeof(f_impl) f __attribute__((alias("f_impl")));

__attribute__((alias("f_impl"))) // CHECK: !/warn/
static int quick_f(); // CHECK: !/warn/

main()
{
	f();
}
