// RUN: %check %s

enum {
	A = 'abc ', // CHECK: /warning: multi-char constant/
	B = 'xyz?', // CHECK: /warning: multi-char constant/
	C = '3251', // CHECK: /warning: multi-char constant/
	D = '500B', // CHECK: /warning: multi-char constant/
};
