// RUN: %ucc %s 2>&1 | grep .; [ $? -ne 0 ]
// RUN: %check %s -std=c89

enum { A, B, }; // CHECK: /warning: trailing comma in enum definition/

int a = { 1, }; // CHECK: /warning: trailing comma in initialiser/

main()
{
	return 0;
}
