// RUN: %check %s -std=c89

enum { A, B, }; // CHECK: /warning: trailing comma in enum definition/

int a = { 1, }; // CHECK: /warning: trailing comma in initialiser/

main()
{
	return 0;
}
