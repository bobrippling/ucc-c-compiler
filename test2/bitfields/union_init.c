// RUN: %check %s
// RUN: %layout_check %s

union U
{
	int : 0;
};

union U u = { 1 }; // CHECK: /warning: excess initialiser for 'union U'/
