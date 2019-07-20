// RUN: %check %s

main()
{
	int idx;

	for(int idx = 0; idx < 3; idx++); // CHECK: /warning: declaration of "idx" shadows local declaration/

	int f();
	if(int idx = f()); // CHECK: /warning: declaration of "idx" shadows local declaration/

	while(int idx = f()); // CHECK: /warning: declaration of "idx" shadows local declaration/
}
