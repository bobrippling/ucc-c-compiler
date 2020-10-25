// RUN: %check -e %s

#define __TIME__ a // CHECK: error: redefining "__TIME__"
