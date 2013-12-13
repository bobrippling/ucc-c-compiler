// RUN: %ucc -fno-ms-extensions -c %s

struct allowed // in C11
{
	union
	{
		int hex;
	};
} a;

int *p_allowed = &a.hex;
