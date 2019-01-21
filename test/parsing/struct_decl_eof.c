// RUN: %check -e %s

struct A, B; // CHECK: error: expecting token ';', got ','
             // CHECK: ^error: expecting token eof, got ','
