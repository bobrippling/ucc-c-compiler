// RUN: %ucc -target x86_64-linux -fdata-sections -ffunction-sections -S -o %t %s
// RUN: grep -F '.section .text.f' %t
// RUN: grep -F '.section .text.g' %t
// RUN: grep -F '.section .text.main' %t
// RUN: grep -F '.section .data.i' %t
// RUN: grep -F '.section .data.j' %t
// RUN: grep -F '.section .data.k' %t

i = 3, j = 4, k = 5;

f()
{
}

g()
{
}

main()
{
}
