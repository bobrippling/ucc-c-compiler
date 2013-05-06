// RUN: %ucc %s -o %t
// RUN: %t

void *func(int x)
{
	char *fp1 = __builtin_frame_address(1);
	fp1 += x;
	return fp1;
}

int main()
{
	void *fp0 = __builtin_frame_address(0);
	void *fp1 = func(0);

	return fp1 == fp0 ? 0 : 1;
}
