// RUN: %ucc %s 2>&1 | grep 'C99 for-init'; [ $? -ne 0 ]
// RUN: %check %s -std=c89

main()
{
	for(int i = 0; i < 10; i++); // CHECK: /warning: use of C99 for-init/
}
