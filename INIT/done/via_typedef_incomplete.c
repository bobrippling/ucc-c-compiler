// RUN: %ucc %s -c
// RUN: %asmcheck %s
typedef int A[];
A a = { 1, 2 }, b = { 3, 4, 5 };
