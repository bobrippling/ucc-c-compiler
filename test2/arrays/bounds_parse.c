main()
{
	int x[4];

	return (-2)[x] // CHECK: /index 2 out of bounds.*2/
		      -2[x]; // CHECK: !/index.*out of bounds.*2/
}
