// RUN: %check -e %s

f(int); // CHECK: note: previous definition
f(); // make sure this doesn't block anything
f(char); // CHECK: error: mismatching definitions of "f"
