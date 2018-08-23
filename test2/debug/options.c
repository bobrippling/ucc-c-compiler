// RUN:   %ucc -g                 %s -S -o- | grep structname
// RUN: ! %ucc -gline-tables-only %s -S -o- | grep structname
// RUN: ! %ucc                    %s -S -o- | grep structname

typedef struct structname {
	int x;
} structname;

int f()
{
	structname a = { .x = 3 };
	return a.x;
}
