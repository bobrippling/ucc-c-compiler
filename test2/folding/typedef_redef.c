// RUN: %check %s

typedef enum abc {
    No,
    One,
    Two
} abc;

typedef enum abc abc; // CHECK: /warning: typedef 'abc' redef/
