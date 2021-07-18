// RUN: %check %s -std=c89

x = sizeof(int []){ 1, 2 }; // CHECK: warning: compound literals are a C99 feature
y = sizeof(struct A { int i, j; }){ 1, 2 }; // CHECK: warning: compound literals are a C99 feature
