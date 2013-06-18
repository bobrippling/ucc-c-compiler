// RUN: %check %s
int x = { 1, 2 }; // CHECK: /warning: 1 excess initialiser for scalar/

char b = { 1, 2, 3 }; // CHECK: /warning: 2 excess initialisers for scalar/

main()
{
	return (int){ 5, 6 }; // CHECK: /warning: 1 excess initialiser for scalar/
}
