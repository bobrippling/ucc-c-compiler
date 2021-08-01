// RUN: %check --only %s -w

int printf(const char *, ...)
  __attribute((format(printf, 1, 2)));

main()
{
  /* ensure that when warnings are off we don't run into problems
   * with printf checking */

  printf("hi\n");
}
