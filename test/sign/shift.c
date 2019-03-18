// RUN: %ucc     %s -S -o- | grep 'sarl'
// RUN: %ucc -DU %s -S -o- | grep 'shrl'
f(unsigned long long);

g(int i)
{
#ifdef U
	f((unsigned)i >> 31); // shr
#else
	f((  signed)i >> 31); // sar
#endif
}
