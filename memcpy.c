typedef unsigned long size_t;
extern void *memcpy(void *, const void *, size_t);

void f(unsigned char *src)
{
	unsigned char *used;

	memcpy(used, src, sizeof used);

	g(used);
}
