// RUN: %check %s -Werror -Wno-error=mismatching-types

int main(void)
{
	char c;
	int *p = &c; // CHECK: warning: mismatching types, initialisation:

	return *p;
}
