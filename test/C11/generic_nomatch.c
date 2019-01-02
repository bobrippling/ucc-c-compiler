// RUN: %ucc %s; [ $? -ne 0 ]
main()
{
	// no match
	f(_Generic(*(struct A *)0, char: 5));
}
