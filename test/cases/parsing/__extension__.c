// RUN: %ocheck 12 %s

__extension__
typedef int int_t;

__extension__
__extension__
__extension__
long long l;

__extension__
__extension__
union U
{
	__extension__ int_t i;
	__extension__ short s;
};

f()
{
	__extension__
	union U abc = { __extension__ __extension__ 10 };

	__extension__ 23;
	__extension__(93);
	__extension__ int_t a = 34;
	__extension__ a = 7;
	__extension__ __extension__ a = 2;

	__extension__ __extension__ 23;
	__extension__ __extension__(93);

	return a + __extension__ __extension__ ({ abc.i; });
}

main()
{
#include "../ocheck-init.c"
	return f();
}
