// RUN: %ucc -DA -c %s; [ $? -ne 0 ]
// RUN: %ucc -DB -c %s; [ $? -ne 0 ]
// RUN: %ucc -DC -c %s; [ $? -ne 0 ]
// RUN: %ucc -DD -c %s; [ $? -ne 0 ]

#ifdef A
f(x)
	int y;
{
}
#endif

#ifdef B
f(int k)
	int j;
{
}
#endif

#ifdef C
g(a, b)
	int a, b, c;
{
}
#endif

#ifdef D
h(a, b)
	int a, c;
{
}
#endif
