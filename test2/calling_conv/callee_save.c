// RUN: %ucc -S -o- %s | grep -F 'not saving reg 1 - callee'
main()
{
	// a() is cached on-stack,
	// b() is cached in ebx
	return a() + b() + c();
}
