// RUN: %check -e %s

typedef struct isn { int x; } isn;

void f(char isn, isn *i) // CHECK: error: expecting token ',', got '*'
{
	// 'char isn' hides the type 'isn'
}
