// RUN: %ucc -c %s
// RUN: [ "`%ucc -S -o- %s | grep -c 'call' | grep -Ec '\b(x|y)\b'`" -eq 2 ]

f(a)
{
	x() || y();
}

