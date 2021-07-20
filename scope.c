int
f1 (int i)
{
    struct foo { int m; };
    if (i > sizeof(struct foo { int m; }))
        return sizeof(struct foo { int m; });
    else
        return sizeof(struct foo);
}

int
f2 (void)
{
    struct foo { int m; };
    for (struct foo { int m; } x;;) // error
        return sizeof(struct foo { int m; });
}

int
f3 (void)
{
    struct foo { int a; };
    while (sizeof(struct foo { int a; }))
        return sizeof(struct foo { int m; });
}

int
f4 (int i)
{
    for (struct foo *x = (struct foo { int m; }*)&i;;) // error
        return x->m + sizeof(struct foo { int m; });
}

void
assert(_Bool b)
{
	void abort(void);
	if(!b) abort();
}

int
main (void)
{
    assert(f1(1) == sizeof(int));
    assert(f2() == sizeof(int));
    assert(f3() == sizeof(int));
    assert(f4(9) == 9 + sizeof(int));
}
