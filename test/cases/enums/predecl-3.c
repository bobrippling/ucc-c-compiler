// RUN: %check -e %s

enum A;

struct Yo
{
	enum A x; // CHECK: /error: incomplete field/
};
