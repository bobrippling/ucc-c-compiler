// RUN: %check -e %s -Werror=implicit-function-declaration

main()
{
	f(); // CHECK: error: implicit declaration of function "f"
}
