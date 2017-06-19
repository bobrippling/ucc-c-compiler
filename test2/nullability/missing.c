// RUN: %check %s -w -Wnullability

int *missing( // CHECK: warning: declaration is missing nullability specifier
		int *missing1) // CHECK: warning: declaration is missing nullability specifier
{
	int *missing2; // CHECK: warning: declaration is missing nullability specifier
	struct {
		int *missing3; // CHECK: warning: declaration is missing nullability specifier
	} a;
}

int *_Nonnull f(int *_Nullable p) // CHECK: !/warn/
{
	static int g; // CHECK: !/warn/

	if(p){
		int *_Nonnull x = (int *_Nonnull)p; // CHECK: !/warn/
		return x;
	}
	return &g;
}

main() // CHECK: !/warn/
{
	int *_Nonnull p = f(0); // CHECK: !/warn/
	__auto_type p2 = f(0); // CHECK: !/warn/

	return *p + *p2;
}
