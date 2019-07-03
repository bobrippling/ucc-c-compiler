// RUN: %check %s
int x = { 1, 2 }; // CHECK: /warning: excess initialiser for 'int'/

char b = { 1, 2, 3 }; // CHECK: /warning: excess initialiser for 'char'/

main()
{
	return (int){ 5, 6 }; // CHECK: /warning: excess initialiser for 'int'/
}
