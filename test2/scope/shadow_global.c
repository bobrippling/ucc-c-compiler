// RUN: %check %s -Wshadow
int idx;
int f(void);

main()
{
	int idx = 0; // CHECK: /warning: declaration of "idx" shadows global declaration/
	int f = 2;   // CHECK: /warning: declaration of "f" shadows global declaration/
}
