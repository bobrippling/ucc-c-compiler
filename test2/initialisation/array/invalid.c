// RUN: %check -e %s

f()
{
	char invalid[]; // CHECK: /error: array has an incomplete size/
}
