// RUN: %ucc -DA %s; [ $? -ne 0 ]
// RUN: %ucc -DB %s; [ $? -ne 0 ]
// RUN: %ucc -DC %s; [ $? -ne 0 ]
// RUN: %ucc -DA %s 2>&1 | grep 'already have'
// RUN: %ucc -DB %s 2>&1 | grep 'already have'
// RUN: %ucc -DC %s 2>&1 | grep 'already have'
main()
{
#ifdef A
	return 1LUL;
#endif
#ifdef B
	return 1LLLU;
#endif
#ifdef C
	return 1Ll;
#endif
}
