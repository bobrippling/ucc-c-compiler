// RUN: %ucc -S -o %t %s -pg
// RUN: grep 'call.*mcount' < %t
// RUN: %ucc -S -o %t %s -pg -mfentry
// RUN: grep 'call.*__fentry__' < %t

void a()
{
}
