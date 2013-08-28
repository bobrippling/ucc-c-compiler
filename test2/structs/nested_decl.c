// RUN: %ucc -fsyntax-only %s
struct node {
	int x;
	struct node;
};
