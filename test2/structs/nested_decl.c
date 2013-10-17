// RUN: %check %s
struct node {
	int x;
	struct node; // CHECK: !/warn/
};
