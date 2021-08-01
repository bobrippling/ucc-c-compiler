// RUN: %check --prefix=c99 %s -std=c99
// RUN: %check --prefix=c89 %s -std=c89

main()
{
	for(int i = 0; i < 10; i++); // CHECK-c89: /warning: use of C99 for-init/
	                             // CHECK-c99: ^ !/warning: use of C99 for-init/
}
