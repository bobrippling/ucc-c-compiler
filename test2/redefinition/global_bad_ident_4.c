// RUN: %check -e %s

static int ch; // CHECK: note: previous definition
static char ch; // CHECK: error: mismatching definitions of "ch"
