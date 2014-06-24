// RUN: %check %s

f() // CHECK: !/warn/
{ // CHECK: !/warn/
	__builtin_unreachable(); // CHECK: !/warn/
} // CHECK: !/warn/

// shouldn't warn about reaching end of main
