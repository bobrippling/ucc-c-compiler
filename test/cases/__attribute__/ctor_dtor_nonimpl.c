// RUN: %ocheck 0 %s

// shouldn't emit ctor reference for x
__attribute((constructor(103))) int x();

int main()
{
	return 0;
}
