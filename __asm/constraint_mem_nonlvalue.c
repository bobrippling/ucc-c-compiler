f(){
	__asm("%0" :: "m"(3)); // 3 isn't an lval, and to match gcc we shouldn't spill it to memory for this
}

g(){
	char t[4] = "abcd";
	int x = 0x64636261;

	__asm__ ("0=%0 1=%1" : "+&m" (t) : "m" (x));
}

h(){
	int a;

	__asm__ ("0=%0 1=%1" : "=m" (a) : "m" (a)); // same value in asm, like matching-constraints but since it's the same lvalue, it's the same memory
}

nocopy(){
	typedef struct { char buf[4096] } Large;

	__asm__ ("0=%0" : : "m" (*(Large *)3)); // since the constraint is "m", we want an lvalue, so we never actually read from &3
}
