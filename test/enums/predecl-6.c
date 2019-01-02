// RUN: %check -e %s

enum X; // CHECK: note: forward declared here

main()
{
	enum X a = 0; // CHECK: error: enum X is incomplete
}

enum X
{
	HI = 1
};
