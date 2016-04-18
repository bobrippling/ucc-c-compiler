// RUN: %archgen %s 'x86_64,x86:/jl .*Lblk/'

// ensure the jump instruction gets the flag

f(int x)
{
	void g();
	if(x < 0)
		g();
}
