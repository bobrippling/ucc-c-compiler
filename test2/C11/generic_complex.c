// RUN: %ucc -DS_OR_U=struct %s; [ $? -ne 0 ]
// RUN: %ucc -DS_OR_U=union %s -o %t
// RUN: %t; [ $? -eq 1 ]

main()
{
	return _Generic(
				*(typeof((*(struct A { int (*i)(union f); } *)0).i))0
				, char: 5, int(S_OR_U f): 1);
}
