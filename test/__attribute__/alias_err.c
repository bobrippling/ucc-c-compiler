// RUN: %check -e %s -target x86_64-linux
// RUN: %check --prefix=darwin -e %s -target x86_64-darwin

f() __attribute((alias("g"))); // CHECK: error: alias "g" doesn't exist
int g;

another() __attribute((alias("f"))) // CHECK: error: alias "another" cannot be a definition
{
}

int h __attribute((alias("g"))); // CHECK: error: alias "h" cannot be a definition
// CHECK-darwin: ^error: __attribute__((alias(...))) not supported on this target (for variables)


int ret4(){ return 4; }
int alias() __attribute((alias("ret4")));

int alias() // CHECK: error: alias "alias" cannot be a definition
{
	return 3;
}
