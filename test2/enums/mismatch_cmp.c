// RUN: %check %s

enum A
{
	X,
	Y = X,
};

enum B
{
	I, J
};

f()
{
	return I == X; // CHECK: warning: enum type mismatch in ==
	// CHECK: ^note: 'enum B' vs 'enum A'
}
