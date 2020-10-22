// RUN: %ocheck 0 %s -fsigned-char
// RUN: %ocheck 0 %s -funsigned-char

extern void _Noreturn abort(void);

int need_rt = 1;

static void runtime_casting(void)
{
	int minus_1 = -1;

#define RT(sp, T, U) T sp = (T)(U)minus_1

	RT(c, long long, signed char);
	RT(s, long long, short);
	RT(i, long long, int);
	RT(l, long long, long);
	RT(L, long long, long long);

	RT(uc, long long, unsigned char);
	RT(us, long long, unsigned short);
	RT(ui, long long, unsigned int);
	RT(uL, long long, unsigned long long);

	if(c != -1) abort();
	if(s != -1) abort();
	if(i != -1) abort();
	if(l != -1) abort();
	if(L != -1) abort();

	if(uc != 255) abort();
	if(us != 65535) abort();
	if(ui != 4294967295) abort();
	if(uL != 1 + 2 * 9223372036854775807ul) abort();

	need_rt = 0;
}


// test constant fold casting:
int sc = (int)(char)-1;

#define DECL(sp, T, U) T sp = (T)(U)-1

DECL(c, long long, signed char);
DECL(s, long long, short);
DECL(i, long long, int);
DECL(l, long long, long);
DECL(L, long long, long long);

DECL(uc, long long, unsigned char);
DECL(us, long long, unsigned short);
DECL(ui, long long, unsigned int);
DECL(uL, long long, unsigned long long);

main()
{
	if(!(__builtin_is_signed(char) ? (sc == -1) : (sc == 255)))
		abort();

	if(c != -1) abort();
	if(s != -1) abort();
	if(i != -1) abort();
	if(l != -1) abort();
	if(L != -1) abort();

	if(uc != 255) abort();
	if(us != 65535) abort();
	if(ui != 4294967295) abort();
	if(uL != 1 + 2 * 9223372036854775807ul) abort();

	runtime_casting();

	return need_rt;
}
