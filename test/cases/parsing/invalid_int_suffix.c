// RUN: %check -e %s

main()
{
	float f = 0f; // CHECK: error: invalid suffix on integer constant
}
