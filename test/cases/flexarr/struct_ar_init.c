// RUN: %check %s
struct A
{
	int n;
	int vals[];
} x[] = {
	// ucc allows this
	1, { 7 },    // CHECK: /warning: initialisation of flexible array/
	2, { 5, 6 }, // CHECK: /warning: initialisation of flexible array/
};

int main(void) {
	__auto_type k = sizeof("z");

	union {
		char b[k];
	} w = { .b = "z" };
	//           ^~~ this would previously ICE because we were checking a flex-array init from char[]/strlit
}
