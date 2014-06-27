void f(size_t n) {
  /* n must be incremented */
  size_t a = sizeof(int[++n]);

  /* n need not be incremented - no change on size */
  size_t b = sizeof(int[++n % 1 + 1]);

  printf("%z, %z, %z\n", a, b, n);
  /* ... */
}
