// RUN: %check -e %s

inline typedef struct { int x; } A; // CHECK: error: inline on non-function

inline typedef int i; // CHECK: error: inline on non-function

inline int glob; // CHECK: error: inline on non-function

inline __auto_type yo = 3; // CHECK: error: inline on non-function

typedef inline struct { int x; } A2; // CHECK: error: inline on non-function

typedef inline int i; // CHECK: error: inline on non-function

int inline glob; // CHECK: error: inline on non-function

__auto_type inline yo2 = 3; // CHECK: error: inline on non-function
