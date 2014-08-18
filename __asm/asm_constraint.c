yo(int x){printf("%d\n", x);}

main()
{
	int i = 5;
	asm("movl %0, %%edi ; call yo" : : "i"(i) : "edi");
}
