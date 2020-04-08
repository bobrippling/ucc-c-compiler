int f(int x){
	__asm("%0 %1" : "=r"(x) : "0"(x));
	return x;
}
