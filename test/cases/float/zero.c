// RUN: %ucc -target x86_64-linux -S -o %t %s
// RUN: %stdoutcheck %s <%t

float f()
{
	// STDOUT: /^f:/
	// STDOUT-NEXT: xorps %xmm0, %xmm0
	return 0;
}

double d()
{
	// STDOUT: /^d:/
	// STDOUT-NEXT: xorpd %xmm0, %xmm0
	return 0;
}

#if 0
long double d()
{
	return 0;
}
#endif
