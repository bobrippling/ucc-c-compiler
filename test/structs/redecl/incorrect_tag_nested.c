// RUN: %check -e %s

union f; // CHECK: note: previous definition here

main()
{
	struct f *p = 0; // CHECK: error: redeclaration of union as struct
}
