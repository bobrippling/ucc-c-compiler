// RUN: %ucc -c -o %t %s
extern char (*(*x)[])();

main()
{
	char c;

	//c = (*x[0])();
	c = (*(*x)[0])();
}
