// RUN: %ucc %s; [ $? -ne 0 ]
// RUN: %ucc -DINIT %s; [ $? -eq 0 ]

main()
{
	typedef int A[];

	A x
#ifdef INIT
		= { 1 }
#endif
	;

	return 3;
}
