// RUN: %ucc -c -o %t %s

typedef struct __FILE FILE;
struct __FILE
{
	void *cookie;
	int x;
	int (*f_close)();
};

int fclose2(FILE *f)
{
	return f->x + 1 ? f->f_close(f->cookie) : 0;
}
