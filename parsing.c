void f(){asm("hi""%0 yo" : "=" "r"(*(int *)3));}
//                         ^~~ ~~~

void g()
{
	__asm("%= %%"); // should get a unique constant and a literal '%'
	__asm("%= %%");
}
