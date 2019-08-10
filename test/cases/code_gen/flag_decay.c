// RUN: %ucc -target x86_64-linux -S -o- %s | grep 'jl .*Lblk'

// ensure the jump instruction gets the flag

f(int x)
{
	void g();
	if(x < 0)
		g();
}
