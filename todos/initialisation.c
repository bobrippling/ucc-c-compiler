int a[  ] = { 0 };
int b[10] = { 0 };

char c[] = "hi there";
char d[] = { 'y', 'o', '\0' };

char *e[] = { a, b };

typedef struct { int x, y; } Point;

Point st_a = { 1, 2 };
Point st_b = { .x = 2, .y = 3 };

union
{
	int i;
	char c;
} un_a = { 0 }, un_b = { 1, 2 };

#ifdef GNU_EXT_1
int f[] = { [1] = 2, [0] = 1 };
int g[] = { [0 ... 10] = 3 };
#endif

#ifdef GNU_EXT_2
int h[] = { [1] = 2, 3, 4, [0] = 1 };
#endif

#ifdef GNU_EXT_2
struct { int x[5]; } st[3] = { [1].x[0] = 3 };
#endif
