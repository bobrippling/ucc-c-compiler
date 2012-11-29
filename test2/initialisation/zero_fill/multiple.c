// RUN: [ `%ucc %s -S -o- | grep 'mov.*2' | wc -l` -eq 1 ]
// RUN: [ `%ucc %s -S -o- | grep 'mov.*0' | wc -l` -eq 3 ]

g()
{
	struct { int a, b; } a = { 2 };
}

h()
{
	int y[3] = { }; // extension, also for structs
}
