// RUN: %layout_check %s

static int x;
static __attribute((used)) int y;

static int f(){}
static __attribute((used)) int g(){}
