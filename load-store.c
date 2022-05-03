struct A {
	char c;
	signed char sc;
	unsigned char uc;
	int i;
};


void struct_and_typed_load_stores()
{
	struct A a;

	a.i = 4;
	a.sc = 9; // [x] byte (and short/long) stores
	a.uc = 2; // [x] non-power-2 offset stores
	a.c = 5;

	short s = 3, *sp = &s; // non-int load/stores
	s = *sp;
}

void f(void *p){}

int caller()
{
	struct A *p = 0;
	f(&p); // [ ]
	//return p->c; // [x] drop "mov r0, r0"
}

void large_imm()
{
	int a = 0x12345678; // movw, movt
}

int main() {
	large_imm();
	caller();
	struct_and_typed_load_stores();
}
