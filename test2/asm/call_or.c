// RUN: %ucc -c %s
// RUN: [ "`%ucc -S -o- %s | grep 'call' | grep -Ec '(x|y)'`" -eq 2 ]

f(a)
{
	x() || y();
}

