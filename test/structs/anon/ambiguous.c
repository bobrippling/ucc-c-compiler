// RUN: %check -e %s -fplan9-extensions

struct A
{
	int a_sub;
};

typedef struct A A;

struct ambig
{
	A;
	int a_sub; // CHECK: error: duplicate member a_sub
};
