// RUN: %check -e %s

int f();

enum
{
	f // CHECK: error: mismatching definitions of "f"
};

main()
{
	f();
	int i = f;
}
