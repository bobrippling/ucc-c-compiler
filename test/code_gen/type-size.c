// RUN: %ucc -target x86_64-linux -S -o %t %s
// RUN: grep -F '.type f,@function' %t
// RUN: grep -F '.size f, .-f' %t
// RUN: grep -F '.type i,@object' %t
// RUN: grep -F '.size i, 4' %t

int i = 3;

void f()
{
}
