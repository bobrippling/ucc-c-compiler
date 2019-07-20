// RUN: %ocheck 0 %s

x_;
nx, ny;

x(){nx++; return x_;}
y(){ny++; return 1;}

f()
{
	x() || y();
}

main()
{
	_Noreturn void abort();
	x_ = 0;
	f();
	if(nx != 1) abort();
	if(ny != 1) abort();

	nx = ny = 0;
	x_ = 1;
	f();
	if(nx != 1) abort();
	if(ny != 0) abort();
}
