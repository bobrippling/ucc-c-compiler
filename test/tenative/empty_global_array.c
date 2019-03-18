// RUN: %check %s -Wtenative-init
// RUN: [ `%ucc -S -o- %s | grep -Ev '\.(type|size)' | grep 'a_global_array' | wc -l` -eq 2 ]
// RUN: [ `%ucc -S -o- %s | grep -Ev '\.(type|size)' | grep 'a_global_struct' | wc -l` -eq 2 ]

// long names to not conflict with local labels, etc etc

int a_global_array[]; // CHECK: !/warning/
int a_global_array[]; // CHECK: !/warning/
int a_global_array[]; // CHECK: /warning: default-initialising tenative definition/

struct A { int i, j; };

struct A a_global_struct; // CHECK: !/warning/
struct A a_global_struct; // CHECK: !/warning/
struct A a_global_struct; // CHECK: !/warning/
struct A a_global_struct; // CHECK: /warning: default-initialising tenative definition/
