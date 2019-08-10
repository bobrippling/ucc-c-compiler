// RUN: %check --only -e %s

enum E
{
	L = -1 // CHECK: error: mismatching definitions of "L"
} L; // CHECK: note: previous definition
