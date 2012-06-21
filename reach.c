extern void exit() __attribute__((noreturn));

f() __attribute__((noreturn))
{
	exit(1);
}

g()
{
	for(;;)
		printf("hi\n");
}

h() __attribute__((noreturn))
{
	while(1)
		break;
}

/*
_Noreturn h()
{

}

noreturn i()
{
}
*/

main()
{
	h();
	//f();
	//g();
}

/*
void fn(void) __attribute__ ((noreturn));
__attribute__ ((noreturn)) void fn(void);
__attribute__ ((noreturn)) void fn(void) {exit(1);}
*/
