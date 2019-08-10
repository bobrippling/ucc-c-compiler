// RUN:   %ucc -target x86_64-linux -S -o- %s -DSIGN=signed   | grep 'cqto'
// RUN: ! %ucc -target x86_64-linux -S -o- %s -DSIGN=unsigned | grep 'cqto'

typedef SIGN long long X;

X f(X a, X b)
{
	return a / b;
}
