// RUN: %ucc -mpreferred-stack-boundary=4 -mstackrealign -S -o %t %s
// RUN: grep -F 'andq $15, %%rsp' %t

// we should realign this regardless
// - could be called from 4-byte stack aligned code
f()
{
}
