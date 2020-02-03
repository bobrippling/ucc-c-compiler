static int private_sum(int* a, int n)
	__attribute__((noinline));

static int private_sum(int* a, int n) {
  return *a + n;
}

int public_sum(int* a, int n) {
  return private_sum(a, n);
}

// gcc -m32 -S -O3 z.c

#ifdef extern_sum
typedef int sum_t(int *a, int n);

sum_t *sum_p(void) {
  return private_sum;
}
#endif
