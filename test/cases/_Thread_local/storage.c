// RUN: %ucc -S -o- -target x86_64-linux %s | %stdoutcheck %s

__thread int tls0;
// STDOUT: .section .tbss
// STDOUT-NOT: /\.section/
// STDOUT: tls0:

_Thread_local int tls1 = 1;
// STDOUT: .section .tdata
// STDOUT-NOT: /\.section/
// STDOUT: tls1:

int f()
{
	static _Thread_local int tls_local;
	// STDOUT: .section .tbss
	// STDOUT-NOT: /\.section/
	// STDOUT: f.static1_tls_local:

	return tls_local;
// STDOUT: f:
}

__thread int array[10];

int g()
{
	return g[3];
}
