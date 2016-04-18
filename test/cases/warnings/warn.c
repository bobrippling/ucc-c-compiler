// RUN: %check --only %s -Wall -Wno-ptr-int-conversion

main()
{
#warning hello      // CHECK: #warning: hello
	return (void *)5; // CHECK: mismatching types, return type
}

#warning hello2 // CHECK: #warning: hello2

f() // CHECK: control reaches end of non-void function f
{
}
