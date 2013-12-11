// RUN: %check %s -Wall

main()
{
#warning hello      // CHECK: /hello/
	return (void *)5; // CHECK: /mismatching return type/
}

#warning timmy // CHECK: /timmy/

f() // CHECK: /control reaches end of non-void function/
{
}
