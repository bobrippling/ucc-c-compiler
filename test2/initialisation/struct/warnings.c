// RUN: %check %s

struct A
{
	int a, b;
} x[] = {
	1, 2 // CHECK: /warning: missing braces for initialisation of sub-object 'struct A'/
};

struct
{
	int a, b;
} l = { 0, 0, 0, 0, 0 }; // CHECK: /warning: excess initialiser/

struct three
{
	int a, b, c;
} trio = { 1 }, // CHECK: /warning: 2 missing initialisers for 'struct three'/
	duo = { 1, 2 }; // CHECK: /warning: 1 missing initialiser for 'struct three'/


struct desig
{
	int a, b, c;
} desig = { .a = 1 }; // CHECK: !/warn/
