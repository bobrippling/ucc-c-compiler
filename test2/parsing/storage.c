// RUN: %check -e %s
enum A { X } const a; // CHECK: !/warn|err/
enum B { Y } static b; // CHECK: !/warn|err/

int x, static y; // CHECK: error: identifier expected after decl (got static)
