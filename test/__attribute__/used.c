// RUN: %layout_check %s
// RUN: %ucc -target x86_64-darwin -S -o %t %s
// RUN: grep -F '.no_dead_strip _y'
// RUN: ! grep -F '.no_dead_strip _x'

static int x;
static __attribute((used)) int y;

static int f(){}
static __attribute((used)) int g(){}
