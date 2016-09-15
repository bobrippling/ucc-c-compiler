// RUN: %ocheck 0 %s

typedef int wchar_t;

char s[] = "\x1b\033\xa";

unsigned char a[] = "a\b\2345\02345\2f\fq\xfq";

char s1[] = "\x12";       // single char with value 0x12 (18 in decimal)
//char s1[] = "\x1234";     // single char with implementation-defined value, unless char is long enough
wchar_t s2[] = L"\x1234"; // single wchar_t with value 0x1234, provided wchar_t is long enough (16 bits suffices)

char o1[] = "\1""1";
char o2[] = "\1111";
char o3[] = "\0x";

unsigned char r1[] = "\xC0"; // 0xc0

char cutoff[] = "\12345";

#define S "\fff"

_Static_assert(sizeof(S) == 4, "");

static void assert(_Bool x, int l, const char *exp)
{
	if(!x){
		_Noreturn void abort(void);
		printf("failed on %d: '%s'\n", l, exp);
		//abort();
	}
}
#define assert(x) assert((x), __LINE__, #x)

int main()
{
	assert(s[0] == 27);
	assert(s[1] == 27);
	assert(s[2] == 10);
	assert(s[3] == 0);

	assert(a[0] == 'a');
	assert(a[1] == '\b');
	assert(a[2] == 0234);
	assert(a[3] == '5');
	assert(a[4] == 023);
	assert(a[5] == '4');
	assert(a[6] == '5');
	assert(a[7] == 2);
	assert(a[8] == 'f');
	assert(a[9] == '\f');
	assert(a[10] == 'q');
	assert(a[11] == 15);
	assert(a[12] == 'q');
	assert(a[13] == 0);

	assert(s1[0] == 0x12);
	assert(s1[1] == 0);

	assert(s2[0] == 0x1234);
	assert(s2[1] == 0);

	assert(o1[0] == 1);
	assert(o1[1] == '1');
	assert(o1[2] == 0);

	assert(o2[0] == 0111);
	assert(o2[1] == '1');
	assert(o2[2] == 0);

	assert(o3[0] == 0);
	assert(o3[1] == 'x');
	assert(o3[2] == 0);

	assert(r1[0] == 0xc0);
	assert(r1[1] == 0);

	assert(S[0] == '\f');
	assert(S[1] == 'f');
	assert(S[2] == 'f');
	assert(S[3] == '\0');

	assert(cutoff[0] == 'S');
	assert(cutoff[1] == '4');
	assert(cutoff[2] == '5');
	assert(cutoff[3] == 0);

	{
		char *oct = "a\0563";
		char *hexterm = "\xagh";
		char *s = "h\xbcdef515abd\03321\012345";
		//         h\275         \03321\n  345

		if(strcmp(oct, "a.3"))
			abort();

		if(strcmp(hexterm, (char[]){ 10, 'g', 'h', 0 }))
			abort();

		if(strcmp(s, "h\275\03321\n345"))
			abort();
	}

	return 0;
}
