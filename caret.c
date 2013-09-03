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

f(int *);

func2()
{
	int abcdef = 0;
	f(1234);
	f(abcdef);

	char c;
	char *p = 0;
	f(&c);
	f(*p);

	f(sizeof(typeof(int)));
}
