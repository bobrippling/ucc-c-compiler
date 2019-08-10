// RUN: %check -e %s

funcs()
{
	return _Generic(funcs, int (*)(): 0, char(): 1); // CHECK: error: function type 'char ()' in _Generic
}
