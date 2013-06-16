// RUN: %ucc -c %s

typedef int int_t;

f()
{
  const int int_t(void);
  int const int_t(void);
  const int (int_t)(void);
  int const (int_t)(void);
  return int_t();
}

// -- other way around

int (func_decl)(void);

g()
{
	typedef int func_decl;
	func_decl a = 3;
	return a;
}
