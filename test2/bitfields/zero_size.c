// RUN: %check %s

struct A
{
	int : 0;
};

struct A a;
struct A b = {};
struct A c = { 1 };
struct A *d = &c;
struct A e[] = { {}, {}, {} };
