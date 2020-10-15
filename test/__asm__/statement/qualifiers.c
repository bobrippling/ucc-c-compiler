// RUN: %check --only -e %s

int main() {
	__asm__ volatile ("");
	__asm__ const (""); // CHECK: Non-volatile qualification after asm (const)
	__asm__ __const (""); // CHECK: Non-volatile qualification after asm (const)

	__asm__ volatile __volatile__ (""); // CHECK: Duplicate qualification after asm (volatile)
}
