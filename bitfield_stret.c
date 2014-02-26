// bit width = 2 -> int
// bit width = 4 -> long
// bit width = 8 -> 2x long
enum
{
	W = 8
};

struct A
{
	int a : W;
	int b : W;
	int c : W;
	int d : W;
	int e : W;
	int f : W;
	int g : W;
	int h : W;
	int i : W;
	int j : W;
	int k : W;
	int l : W;
	int m : W;
	int n : W;
	int o : W;
	int p : W;
// perl -e 'print "\tint " . chr(97 + $_) . " : 1;\n" for(1 ... 26)'
};

struct A f()
{
	return (struct A){};
}
