// RUN: %asmcheck %s
// both of these should generate a testl %eax, %eax
f(int i)
{
	return !!i; // setne aka setnz
}

g(int i)
{
	return !i; // sete aka setz
}
