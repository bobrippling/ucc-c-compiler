// RUN: %debug_scope %s
int printf(const char *, ...) __attribute__((format(printf, 1, 2)));

int f(int arg) // SCOPE: arg ret
{ // SCOPE: arg ret
	int ret = 5 + arg; // SCOPE: arg ret
	for(int i = 0; i < 3; i++){ // SCOPE: arg ret i
		printf("%d\n", i); // SCOPE: arg ret i
	} // SCOPE: arg ret
	return ret; // SCOPE: arg ret
} // SCOPE: arg

main()
{
	return f(2);
}
