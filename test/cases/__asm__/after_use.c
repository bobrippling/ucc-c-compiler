// RUN: %check -e %s

void f();

int main()
{
	f();
}

__typeof(f) f __asm__("hi"); // CHECK: error: cannot annotate "f" with an asm() label after use
