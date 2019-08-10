// RUN: %check --only --prefix=noext  %s
// RUN: %check --only --prefix=ext -e %s -fplan9-extensions

struct A1
{
	int x; // CHECK-ext: note: duplicate of this member, and ...
// CHECK-ext: ^note: ... this member
};

struct A2
{
	int x; // CHECK-ext: note: ... this member
};

struct B // CHECK-ext: error: struct B contains duplicate member "x"
{
	int y; // ambiguous in plan9/ms extensions
	struct A1; // CHECK-noext: warning: unnamed member 'struct A1' ignored (untagged would be accepted in C11)
// CHECK-ext: ^warning: tagged struct 'struct A1' is a Microsoft/Plan 9 extension
	struct A2; // CHECK-noext: warning: unnamed member 'struct A2' ignored (untagged would be accepted in C11)
// CHECK-ext: ^warning: tagged struct 'struct A2' is a Microsoft/Plan 9 extension
} p1;

struct c // CHECK-ext: error: struct c contains duplicate member "x"
{
	int x; // CHECK-ext: note: duplicate of this member, and ...
	struct A1; // CHECK-noext: warning: unnamed member 'struct A1' ignored (untagged would be accepted in C11)
// CHECK-ext: ^warning: tagged struct 'struct A1' is a Microsoft/Plan 9 extension
} p2;


int main()
{
	p1 = (struct B){
		1,
		2, // CHECK-noext: warning: excess initialiser for 'struct B'
// CHECK-ext: ^warning: missing braces for initialisation of sub-object 'struct A1'
		3
// CHECK-ext: ^warning: missing braces for initialisation of sub-object 'struct A2'
	};
	p2 = (struct c){
		1,
		2 // CHECK-noext: warning: excess initialiser for 'struct c'
// CHECK-ext: ^warning: missing braces for initialisation of sub-object 'struct A1'
	};
}
