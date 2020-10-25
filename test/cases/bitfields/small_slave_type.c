// RUN: %ocheck 0 %s

void abort(void);

typedef struct {
	short x:12;
	char y:4;
} T;

typedef struct {
	short x:12;
	char y:1;
} U;

int main(void)
{
	T t = { 0 };
	t.y = 1;

	if(t.x != 0)
		abort();
	if(t.y != 1)
		abort();

	U u = { 0 };
	u.y = 1;

	if(u.x != 0)
		abort();
	if(u.y != -1)
		abort();

}
