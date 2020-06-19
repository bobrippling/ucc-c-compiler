__attribute((noinline))
f(int a, int b) {
	int q = a;

	return 3 + q + b;
}

main() {
	int x = 2;
	return f(x, 3);
}
