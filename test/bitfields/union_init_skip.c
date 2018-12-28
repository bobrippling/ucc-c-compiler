// RUN: %layout_check %s

union U
{
	int : 0; // skipped
	int a : 1;
};

union U u = { 1 };
