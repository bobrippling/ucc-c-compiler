// RUN: %ucc -S -o- %s | grep -F '56(%%rbp)'
// RUN: %ocheck 0 %s -DPROGRAM

#ifdef PROGRAM
int before(int a, int b, int c, int d, int e, int f, int g)
{
	return a+b+c+d+e+f+g;
}
#endif

__attribute((cdecl))
int attributed(int a, int b, int c, int d, int e, int f, int g)
{
	return a+b+c+d+e+f+g;
}

#ifdef PROGRAM
int after(int a, int b, int c, int d, int e, int f, int g)
{
	return a+b+c+d+e+f+g;
}
#endif

#ifdef PROGRAM
void assert_eq(int a, int b)
{
	if(a != b){
		_Noreturn void abort(void);
		abort();
	}
}

int main()
{
	assert_eq(before(1,2,3,4,5,6,7), 28);
	assert_eq(attributed(1,2,3,4,5,6,7), 28);
	assert_eq(after(1,2,3,4,5,6,7), 28);
}
#endif
