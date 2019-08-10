// RUN: %ucc -target x86_64-linux -S -o %t %s
// RUN:   grep 'testl %%eax, %%eax' %t
// RUN: ! grep 'cmp' %t

// both of these should generate a testl %eax, %eax

f(int i)
{
	return !!i; // setne aka setnz
}

g(int i)
{
	return !i; // sete aka setz
}
