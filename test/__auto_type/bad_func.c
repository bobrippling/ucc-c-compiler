// RUN: %check -e %s
__auto_type g() // CHECK: error: __auto_type without initialiser
{
	return 3;
}
