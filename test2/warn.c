// RUN: %ucc -Wall %s 2>&1 | %check %s

main()
{
#warning hello      // CHECK: /hello/
	return (void *)5; // CHECK: /mismatching return type/
}

#warning timmy // CHECK: /timmy/

f()
{ // CHECK: /control reaches end of void function/
}
