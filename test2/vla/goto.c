// RUN: %check -e %s

main()
{
	int n = 5;
	goto a; // CHECK: error: goto enters scope of variably modified variable
begin:
	{
		int (*abc)[n]; // CHECK: note: variable "abc"
		goto begin2;
a:;
	}

	goto b; // CHECK: error: goto enters scope of variably modified variable
begin2:
	{
		int xyz[n]; // CHECK: note: variable "xyz"
		goto end;
b:;
	}

	goto begin;
end:;
}
