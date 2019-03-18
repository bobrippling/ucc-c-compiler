// RUN: %ucc -o %t %s
// RUN: %t; [ $? -eq 2 ];
f(){}
main()
{
	return 1 + ({f(); 1;});
}
