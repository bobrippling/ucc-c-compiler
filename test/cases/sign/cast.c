// RUN: %ucc -o %t %s
// RUN: %t; [ $? -eq 253 ]

main()
{
	return (unsigned)-3;
}
