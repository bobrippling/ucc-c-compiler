// RUN: %layout_check %s

union U
{
	struct S
	{
		int i, j, k;
	} a;
	int i;
	char *p;
};

struct S st;

sz_U  = sizeof(union U);       // 16
sz_S  = sizeof((union U){}.a); // 12
sz_st = sizeof(st); // 12
