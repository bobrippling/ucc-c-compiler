// RUN: %check -e %s

main()
{
	int n = 5;
	goto a; // CHECK: error: goto enters scope of variably modified declaration
begin:
	{
		int (*abc)[n]; // CHECK: note: variable "abc"
		goto begin2; // CHECK: !/error/
a:;
	}

	goto b; // CHECK: error: goto enters scope of variably modified declaration
begin2:
	{
		int xyz[n]; // CHECK: note: variable "xyz"
		goto end; // CHECK: !/error/
b:;
	}

	goto begin; // CHECK: !/error/
end:;
}
