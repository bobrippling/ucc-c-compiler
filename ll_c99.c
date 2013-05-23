// RUN: %check -c %s

long long l; // CHECK: /warning: "long long" is a C99 feature/

f()
{
	return 5LL; // CHECK: /warning: "long long" is a C99 feature/
}
