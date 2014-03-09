// RUN: %ocheck 12 %s

__extension__
typedef int int_t;


__extension__
union U
{
	__extension__ int_t i;
	__extension__ short s;
};

f()
{
	__extension__
	union U abc = { __extension__ 10 };

	__extension__ 23;
	__extension__(93);
	__extension__ int_t a = 34;
	__extension__ a = 2;

	return a + __extension__ ({ abc.i; });
}

main()
{
	return f();
}
