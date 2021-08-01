// RUN: %check %s

// extern
extern inline int f()
{
	static int i;
	return i++;
}

// static
static inline int h()
{
	static int i;
	return i++;
}

// neither
inline int g()
{
	static int i; // CHECK: warning: mutable static variable in pure-inline function - may differ per file
	return i++;
}

// neither, but const
inline int g2()
{
	static const int i = 3; // CHECK: !/warn/
	return i;
}

main()
{
	return f() + g() + h();
}
