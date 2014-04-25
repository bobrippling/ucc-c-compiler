// RUN: %check -e %s

f(int x)
{
#ifndef __STDC_NO_VLA__
	TODO
  _Generic(n, char [n]: 0);
#endif

  _Generic(0, struct A: 0); // CHECK: error: incomplete type 'struct A' in _Generic
  _Generic(0, void: 0); // CHECK: error: incomplete type 'void' in _Generic

  _Generic(0, int (void): 0); // CHECK: error: function type 'int (void)' in _Generic

  _Generic(0, int: 0, signed: 0); // CHECK: error: duplicate type in _Generic: int
  _Generic(0, default: 0, default: 1); // CHECK: error: second default for _Generic

  _Generic(0, char: 0); // CHECK: error: no type satisfying int
}
