// RUN: %ucc -target x86_64-linux -fdata-sections -ffunction-sections -S -o %t %s
// RUN: test 1 -eq $(grep -cF '.section .text.f' %t)
// RUN: test 1 -eq $(grep -cF '.section .text.g' %t)
// RUN: test 1 -eq $(grep -cF '.section .text.main' %t)
// RUN: test 1 -eq $(grep -cF '.section .data.i' %t)
// RUN: test 1 -eq $(grep -cF '.section .data.j' %t)
// RUN: test 1 -eq $(grep -cF '.section .data.k' %t)

i = 3, j = 4, k = 5;

f(int a)
{
	return a ? 2 : 4;
}

g()
{
}

main()
{
}
