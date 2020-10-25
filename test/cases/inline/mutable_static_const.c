// RUN: %check %s

inline void f(void)
{
	static const int yo[] = { 1, 2 }; // CHECK: !/warning: mutable static variable/
}
