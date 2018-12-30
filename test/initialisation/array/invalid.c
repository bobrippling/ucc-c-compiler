// RUN: %check -e %s

f()
{
	char invalid[]; // CHECK: error: "invalid" has incomplete type 'char[]'
}
