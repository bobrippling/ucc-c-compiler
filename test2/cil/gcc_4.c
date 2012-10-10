// RUN: %ucc -o %t %s && (%t; [ $? -eq 5 ])
main()
{
	return ({goto L; 0;}) && ({L: 5;});
}
