// RUN: %ucc %s -o %t -g

// need to ensure this links with debug info, since f() isn't emitted
inline void f(void)
{
}

main()
{
}
