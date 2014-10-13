// RUN: %check -e %s

static int a; // CHECK: !/error/
static int a; // CHECK: !/error/
static int a; // CHECK: !/error/

int b; // CHECK: note: previous definition
static int b; // CHECK: error: static redefinition of non-static "b"

static int c; // CHECK: note: previous definition
int c; // CHECK: error: non-static redefinition of static "c"
int c;

extern int d;
int d; // CHECK: note: previous definition
static int d; // CHECK: error: static redefinition of non-static "d"
