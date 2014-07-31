// RUN: %archgen %s 'x86_64:/cqto/'  -DSIGN=signed
// RUN: %archgen %s 'x86_64:!/cqto/' -DSIGN=unsigned

typedef SIGN long long X;

X f(X a, X b)
{
	return a / b;
}
