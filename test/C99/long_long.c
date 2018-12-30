// RUN: %check %s -std=c89
// RUN: echo 'long long l;' | %ucc - -std=c99 -fsyntax-only 2>&1 | grep .; [ $? -ne 0 ]

long long l; // CHECK: /warning: long long is a C99 feature/

f()
{
	return 5LL; // CHECK: /warning: long long is a C99 feature/
}
