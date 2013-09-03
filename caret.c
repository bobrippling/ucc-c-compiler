void *f_name(void *p_name, int a_name)
{
  // operator
  p_name++    ;
  ++p_name    ;
  return p_name += a_name;
}

// initialisation
int *p_name = 0;

// init 2
func()
{
  void q();
  (void)(1 ? f_name : q);
}

main()
{
  // argument
  f_name((void *)0, (int *)5);

  int integer = 2;
  f_name(integer, 3);
}
