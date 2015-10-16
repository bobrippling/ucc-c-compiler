// RUN: %check %s -Wunused-function
// RUN: ! %ucc -S -o- %s | grep 'f:'

static int f() // CHECK: warning: unused function 'f'
{
	return 3;
}

int main()
{
}
