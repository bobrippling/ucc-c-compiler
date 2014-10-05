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

	(void)(X == X); // CHECK: !/warn/
	(void)(X == Y); // CHECK: !/warn/
	(void)(Y == X); // CHECK: !/warn/
	(void)(I == J); // CHECK: !/warn/
	(void)(J == I); // CHECK: !/warn/

	return I == X; // CHECK: warning: enum type mismatch in ==
	// CHECK: ^note: 'enum B' vs 'enum A'
}
