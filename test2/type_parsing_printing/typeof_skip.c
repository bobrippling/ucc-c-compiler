// RUN: %ucc -emit=print %s 2>/dev/null | grep 'typeof' | %output_check -w "/typeof\(int \*\) ip.*/" "/typeof\(int \*\) \*a1.*/" "/typeof\(int \*\) a2\[2\].*/" "/typeof\(int \*\) a3\(\).*/" "typeof(expr: identifier) (aka 'long *') abc()" "typeof(expr: identifier) (aka 'long *') xyz()"

long *x;

__typeof(int *) ip;

__typeof(int *) *a1;
__typeof(int *) a2[2];
__typeof(int *) a3();

auto abc() -> __typeof(x)
{
}

__typeof(x) xyz()
{
}
