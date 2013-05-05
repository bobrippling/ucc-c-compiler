// RUN: ! %ucc %s 2>&1 | grep .
// RUN: %check -std=c89 %s

enum { A, B, }; // CHECK: /warning: trailing comma in enum definition/

int a = { 1, }; // CHECK: /warning: trailing comma in initialiser/

main()
{
	return 0;
}
