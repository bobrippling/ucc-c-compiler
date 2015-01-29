typedef unsigned long long uint64_t;
extern int printf(const char *, ...);

typedef struct
{
	uint64_t h, l;
} i128;

void add(
		i128 const *const l,
		i128 const *const r,
		i128       *const out)
{
	*out = *l;

	__asm(
#ifdef __UCC__
			"add %2, %0\n\t"
			"adc %3, %1"
			: "+rm"(out->l)
			, "+rm"(out->h)
			: "r"(r->l)
			, "r"(r->h)
#else
			"add %[r_l], %[out_l]\n\t"
			"adc %[r_h], %[out_h]"
			: [out_l] "+rm"(out->l)
			, [out_h] "+rm"(out->h)
			: [r_l] "r"(r->l)
			, [r_h] "r"(r->h)
#endif
			);
}

void print(i128 const *i)
{
	printf("0x%llx%llx", i->h, i->l);
}

int main()
{
	i128 a = { 0, 1 };
	i128 b = { 0, 5 };

	i128 c;
	add(&a, &b, &c);

	print(&a);
	printf(" + ");
	print(&b);
	printf(" = ");
	print(&c);
	printf("\n");
}
