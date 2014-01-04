// RUN: %ucc %s -c
// RUN: %asmcheck %s
typedef int A[];
A ent1 = { 1, 2 }, ent2 = { 3, 4, 5 };
