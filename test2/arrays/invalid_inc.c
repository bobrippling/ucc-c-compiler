// RUN: %ucc -c %s; [ $? -ne 0 ]
// RUN: %check %s

f(int (*x)[]) // CHECK: /error: incomplete array size/
              // currently the error is on the decl
{
	return *x[1];
}
