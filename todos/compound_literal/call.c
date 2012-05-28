void f()
{
  int *p;
  extern int g(void);
  {
    p = &(int) {g()};
    *p = 1;   //OK
  }
  // p points to deallocated
  // stack space
  *p = 2;   //BAD
}
