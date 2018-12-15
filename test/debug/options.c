// RUN:   %ucc -g                 %s -S -o- | grep structname
// RUN: ! %ucc -gline-tables-only %s -S -o- | grep structname
// RUN: ! %ucc                    %s -S -o- | grep structname
//
// RUN: ! %ucc    -gcolumn-info    %s -S -o- | grep -F '.loc 1 13 5'
// RUN:   %ucc -g -gcolumn-info    %s -S -o- | grep -F '.loc 1 13 5'
// RUN:   %ucc -g -gno-column-info %s -S -o- | grep    '.loc 1 13$'

typedef struct structname {
	int x;
} structname;

int f()
{
	structname a = { .x = 3 };
	return a.x;
}
