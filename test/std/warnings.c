// RUN: %check %s -std=c89

typedef int x; // CHECK: note: other definition here
typedef int x; // CHECK: warning: typedef 'x' redefinition is a C11 extension

enum
{
	A,
	B, // CHECK: warning: trailing comma in enum definition
};

struct
{
	struct
	{
		int i;
	}; // CHECK: /warning: unnamed member 'struct <.*>' is a C11 extension/
} a;

main()
{
	for(int i = 0; i < 10; i++){ // CHECK: warning: use of C99 for-init
		;
	}

	long long i = a.i + 1; // CHECK: warning: long long is a C99 feature
	// CHECK: ^ warning: mixed code and declarations

	int ar[] = { f() }; // CHECK: warning: aggregate initialiser is not a constant expression

	int vla[i]; // CHECK: warning: variable length array is a C99 feature

	short *p = (short [2]){ 1, 2 }; // CHECK: warning: compound literals are a C99 feature

	return i;
}
