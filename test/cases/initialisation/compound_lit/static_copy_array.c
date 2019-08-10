// RUN: %check -e %s

struct A
{
	int i, j;
};

struct Nested
{
	struct A a;
	struct A as[2];
};

struct Nested ents = { // CHECK: error: global brace initialiser not constant
	(struct A){ 1, 2 },
	(struct A[]){ // array copy, not struct copy - error
		(struct A){ 3, 4 },
		(struct A){ 5, 6 },
	}
};
