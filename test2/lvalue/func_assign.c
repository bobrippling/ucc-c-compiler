// RUN: %ucc %s; [ $? -ne 0 ]
f();

main()
{
	f = 2;
}
