// RUN: %check -e %s

main()
{
	struct A { int i, j; } a;
	while(a){ // CHECK: /error: struct involved in while/
	}
}
