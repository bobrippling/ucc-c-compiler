// RUN: %check -e --only          %s -target x86_64-linux
// RUN: %check -e --prefix=darwin %s -target x86_64-darwin
//                ^~~~~~~~~~~~~~~ no `--only` since we'd just dup all the existing ones

f() __attribute((alias("g"))); // CHECK: error: alias "g" doesn't exist
int g;

struct A {
	int x __attribute((section("abc"))); // CHECK: warning: section attribute on member
	int y __attribute((alias("g"))); // CHECK: warning: alias attribute on member
};

another() __attribute((alias("f"))) // CHECK: error: alias "another" cannot be a definition
{
	return 3;
}

int h __attribute((alias("g"))); // CHECK: error: alias "h" cannot be a definition
// CHECK-darwin: ^error: __attribute__((alias(...))) not supported on this target (for variables)

extern void undefined();
void undefined_alias() __attribute((alias("undefined"))); // CHECK: error: target "undefined" of alias "undefined_alias" isn't a definition

int ret4(){ return 4; }
int alias() __attribute((alias("ret4")));

int alias() // CHECK: error: alias "alias" cannot be a definition
{
	return 3;
}

void abc() __attribute((section("abc_sec"))) {}
void xyz() __attribute((alias("abc"), section("abc_sec")));

void abc2() __attribute((section("abc_sec2"))) {}
void xyz2() __attribute((alias("abc2"), section("abc_sec"))); // CHECK: error: alias and target have different sections

void fn(){}
__attribute((alias("fn"), section("fn_section"))) void fn_section(); // CHECK: error: alias and target have different sections
int var;
__attribute((alias("var"), section("var_section"))) extern int var_section; // CHECK: error: alias and target have different sections
