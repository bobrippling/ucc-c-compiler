// RUN: %ocheck 0 %s -fno-inline-functions -fno-semantic-interposition
// RUN: %check --only %s -finline-functions -fno-semantic-interposition

void *func(int x)
{
	char *fp1 = __builtin_frame_address(1); // CHECK: warning: inlining function with call to __builtin_frame_address
	// CHECK: ^warning: calling '__builtin_frame_address' with a non-zero argument is unsafe
	fp1 += x;
	return fp1;
}

int main()
{
	void *fp0 = __builtin_frame_address(0);
	void *fp1 = func(0);

	return fp1 == fp0 ? 0 : 1;
}

void unrelated()
{
	void *p = __builtin_return_address(1); // CHECK: warning: calling '__builtin_return_address' with a non-zero argument is unsafe
}
