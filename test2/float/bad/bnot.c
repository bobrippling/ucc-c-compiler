// RUN: %check -e %s

f()
{
	~1.0f; // CHECK: error: ~ requires an integral expression (not "float")
}
