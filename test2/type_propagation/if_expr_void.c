// RUN: %ucc %s; [ $? -ne 0 ]
// RUN: %ucc %s 2>&1 | %check %s

main()
{
	__typeof__(*(0 ? (int*)0 : (void*)1)) x; // CHECK: /pointer to incomplete type void/

	f(*x);
}
