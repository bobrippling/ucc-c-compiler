// RUN: %ucc -c %s; [ $? -ne 0 ]
// RUN: %ucc %s 2>&1 | %check %s

f(int (*x)[]) // CHECK: /error: incomplete array size/
              // currently the error is on the decl
{
	return *x[1];
}
