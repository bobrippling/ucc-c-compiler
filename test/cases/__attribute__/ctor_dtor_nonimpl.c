// RUN: ! %ucc -S -o- %s | grep notpresent

// shouldn't emit ctor reference for notpresent
__attribute((constructor(103))) int notpresent();

int main()
{
	return 0;
}
