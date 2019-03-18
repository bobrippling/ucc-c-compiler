// RUN: %check -e %s

main()
{
	struct { void (*exit)(enum E { EXIT_FAILURE = 1 }); } x; // CHECK: warning: declaration of 'enum E' only visible inside function

	x.exit(EXIT_FAILURE); // CHECK: error: undeclared identifier "EXIT_FAILURE"
}
