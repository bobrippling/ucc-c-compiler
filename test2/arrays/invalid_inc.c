// RUN: %ucc %s; [ $? -ne 0 ]
// RUN: %ucc %s | %check %s

f(int (*x)[])
{
	return *x[1]; // CHECK: /error: incomplete array size/
}
