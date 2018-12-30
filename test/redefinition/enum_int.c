// RUN: %check -e %s

extern enum { A, B, C } E; // CHECK: note: previous definition

int E = B; // CHECK: error: mismatching definitions of "E"
