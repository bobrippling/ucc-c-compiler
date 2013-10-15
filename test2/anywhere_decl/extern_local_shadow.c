// RUN: %check %s

f()
{
	extern int j;

	f();

	extern int j; // CHECK: /warning: declaration of "j" shadows local declaration/
}
