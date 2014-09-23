// RUN: %check -e %s
f()
{
	static int ar2[]; // CHECK: error: "ar2" has incomplete type 'int[]'
}
