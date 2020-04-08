#ifndef __GCC_ASM_FLAG_OUTPUTS__
#error hi
#endif

int f()
{
	int q;
	__asm__("hi" : "=@ccz"(q));
	//                  ^ condition
	//               ^~~~ flag
	return q;
}
