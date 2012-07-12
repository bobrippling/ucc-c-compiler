extern int printf();
extern void __attribute__((noreturn)) exit(), abort();

void f() __attribute__((noreturn));

void f()
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
			for(;;)
				break;
			break;
		}
}

void _Noreturn i()
{
	exit(0);
}

_Noreturn void j()
{
	do;while(1);
}

main()
{
	i();
	j();
	abort(); //printf("shouldn't see this\n");
}

/*
void fn(void) __attribute__ ((noreturn));
__attribute__ ((noreturn)) void fn(void);
__attribute__ ((noreturn)) void fn(void) {exit(1);}
*/
