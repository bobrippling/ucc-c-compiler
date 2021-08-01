// RUN: %check -e %s

struct A __attribute(())
{ // CHECK: error: expecting token ';', got '{'
	char c;
};
