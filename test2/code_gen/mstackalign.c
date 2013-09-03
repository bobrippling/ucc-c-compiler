// RUN: %ucc -mpreferred-stack-boundary=4 -mstackrealign -S -o %t %s
// RUN: grep -F 'andq $15, %%rsp' %t
// RUN: grep -F 'subq $16, %%rsp' %t

f()
{
}
