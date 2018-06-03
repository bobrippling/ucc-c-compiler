// RUN: %check %s -Werror -Wno-error=incompatible-pointer-types

int main(void)
{
	char c;
	int *p = &c; // CHECK: warning: mismatching types, initialisation

	return *p;
}
