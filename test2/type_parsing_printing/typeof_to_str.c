// RUN: %ucc %s -c
// RUN: %ucc -emit=print %s | grep -F 'typeof(typeof(size_t)) a'

typedef long unsigned size_t;
__typeof(__typeof(size_t)) a;
