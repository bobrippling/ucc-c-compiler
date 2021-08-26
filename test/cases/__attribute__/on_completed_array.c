// Bug: the section (in fact the whole decl) wasn't emitted as we dropped the "used" attribute when completing the array type

// The fix was:
// 1) array type completion preserves attributes
// 2) all non-typrop attributes get hoisted to the decl, as an improvement

// RUN: %layout_check %s

__attribute((used))
static
const char __attribute((section("sec"))) x[] = "hello";
