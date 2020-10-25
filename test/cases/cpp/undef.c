// RUN: %check %s -E -Wundef

#define A    entry
#define F(a) a + 1

#undef A
#undef F
#undef B // CHECK: warning: macro "B" not defined
