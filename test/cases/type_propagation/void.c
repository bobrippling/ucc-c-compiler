// RUN: %check -e --prefix=a %s -DA
// RUN: %check -e --prefix=b %s -DB

main()
{
#ifdef A
	void a; // CHECK-a: error: "a" has incomplete type 'void'
#else
	void b[2]; // CHECK-b: error: array has incomplete type 'void'
#endif
}
