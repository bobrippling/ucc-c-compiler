// RUN: %check %s
// RUN: %ocheck 0 %s

int main()
{
	unsigned x = (unsigned char)-5; // CHECK: warning: implicit cast changes value from -5 to 251

	if(x != 251)
		abort();

	return -5u < 10;
}
