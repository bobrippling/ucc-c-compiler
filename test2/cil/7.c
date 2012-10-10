// RUN: %ucc -o %t %s && ( %t; [ $? -eq 2 ] )

main()
{
	return ((int []){1,2,3,4})[1];
}
