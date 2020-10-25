// RUN: %check --only %s

int f(){return 3;}

int main()
{
	int f() __attribute__((warn_unused_result)); // CHECK: warning: declaration of "f" after definition is ignored
	f(); // CHECK: warning: unused expression
}
