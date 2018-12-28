// RUN: %check %s

struct node
{
	int x;
	struct node; // CHECK: /warning: unnamed member 'struct node' ignored/
};
