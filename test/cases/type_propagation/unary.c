// RUN: %check --only -e %s -Wno-unused-expression

void f(
	int *scalar,
	float arithmetic, // +scalar
	short integer // +scalar +arithmetic
)
{
	+integer;
	-integer;
	~integer;
	!integer;

	+scalar; // CHECK: error: + requires an arithmetic type (not "int *")
	-scalar; // CHECK: error: - requires an arithmetic type (not "int *")
	~scalar; // CHECK: error: ~ requires an integral type (not "int *")
	!scalar;

	+arithmetic;
	-arithmetic;
	~arithmetic; // CHECK: error: ~ requires an integral type (not "float")
	!arithmetic;
}
