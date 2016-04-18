// RUN: %check -e %s

enum an_enum // CHECK: note: previous definition here
{
	X
};

enum an_enum // CHECK: error: redefinition of enum in scope
{
	REDEFINED
};
