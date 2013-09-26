// RUN: %check -e %s

int f(); // CHECK: error: mismatching definitions of "f"

enum
{
	f
};

main()
{
	f();
	int i = f;
}
