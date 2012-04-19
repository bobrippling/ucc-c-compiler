void f(  int a[static const volatile 10] ) {}
int main() {
	int b[5];
	f(b);
}
