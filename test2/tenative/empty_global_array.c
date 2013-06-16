// RUN: %check %s -Wtenative-init
// RUN: [ `%ucc -S -o- %s | grep 'ar' | wc -l` -eq 2 ]
// RUN: [ `%ucc -S -o- %s | grep 'st' | wc -l` -eq 2 ]

int ar[]; // CHECK: !/warning/
int ar[]; // CHECK: !/warning/
int ar[]; // CHECK: /warning: default-initialising tenative definition/

struct A { int i, j; };

struct A st; // CHECK: !/warning/
struct A st; // CHECK: !/warning/
struct A st; // CHECK: !/warning/
struct A st; // CHECK: /warning: default-initialising tenative definition/
