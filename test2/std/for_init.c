// RUN: %ucc %s 2>&1 | grep .; [ $? -ne 0 ]
// RUN: %check -std=c89 %s

main()
{
	for(int i = 0; i < 10; i++); // CHECK: /warning: use of C99 for-init/
}
