// RUN: %ucc -o %t %s
// RUN: %t; [ $? -eq 8 ]
main()
{
	int a[2];

	return sizeof a; // 8
}
