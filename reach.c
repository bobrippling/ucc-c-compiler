extern int printf();
extern void __attribute__((noreturn)) exit(), abort();

void
f() __attribute__((noreturn))
{
	exit(1);
}

g()
{
	for(;;)
		printf("hi\n");
}

_Noreturn void h()
{
	while(1)
		switch(1){
			break;
		}
}

void _Noreturn i()
{
	abort();
}

main()
{
	h();
	printf("shouldn't see this\n");
	//f();
	//g();
}

/*
void fn(void) __attribute__ ((noreturn));
__attribute__ ((noreturn)) void fn(void);
__attribute__ ((noreturn)) void fn(void) {exit(1);}
*/
