// RUN:   cc -DIMPL -c -o %s.impl.o %s
// RUN: %ucc        -c -o %s.call.o %s
// RUN: cc -o %t %s.call.o %s.impl.o
// RUN: %t; [ $? -eq 72 ]
//
// RUN: %ucc -DIMPL -c -o %s.impl.o %s
// RUN:   cc        -c -o %s.call.o %s
// RUN: cc -o %t %s.call.o %s.impl.o
// RUN: %t; [ $? -eq 72 ]
// RUN: rm -f %s.impl.o %s.call.o

int f(
		int a, int b, int c,
		int d, int e, int f,
		int g, int h);

double g(
		double a, double b, double c,
		double d, double e, double f,
		double g, double h);

#if defined(IMPL) || defined(ALL)
int f(
		int a, int b, int c,
		int d, int e, int f,
		int g, int h)
{
    return a+b+c+d+e+f+g+h;
}

double g(
		double a, double b, double c,
		double d, double e, double f,
		double g, double h)
{
    return a+b+c+d+e+f+g+h;
}
#endif

#if !defined(IMPL) || defined(ALL)
main()
{
    int i = f(1,2,3,4,5,6,7,8);
    double x = g(1,2,3,4,5,6,7,8);
    return i+x;
}
#endif
