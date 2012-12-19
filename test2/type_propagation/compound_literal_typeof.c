// RUN: %ucc %s -o %t
// RUN: %t; [ $? -eq 12 ]

main()
{
	typeof(int[]){1,2,3} a;

	return sizeof a;
}
