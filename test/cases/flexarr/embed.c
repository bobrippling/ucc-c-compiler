// RUN: %check %s

typedef struct {
	int n;
	int p[];
} flex;

flex ar1[2]; // CHECK: warning: embedded flexible-array as array element

flex x; // CHECK: !/warn/

struct A
{
	flex abc; // CHECK: warning: embedded flexible-array as nested in struct
};

struct B
{
	int n;
	int p[];
} ar2[2]; // CHECK: warning: embedded flexible-array as array element
