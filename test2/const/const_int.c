// RUN: %ucc %s; [ $? -ne 0 ]
// RUN: %ucc %s 2>&1 | %check %s

main()
{
	typedef const kint;
	kint x;

	x = 2; // CHECK: /can't modify const expression/
}
