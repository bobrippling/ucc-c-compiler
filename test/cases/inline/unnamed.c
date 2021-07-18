// RUN: %check -e %s

main()
{
	inline(); // CHECK: error: unnamed function declaration
}
