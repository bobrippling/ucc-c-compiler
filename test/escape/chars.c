// RUN: %ocheck 0 %s
// RUN: %check %s
int printf(const char *, ...) __attribute__((format(printf, 1, 2)));

_Static_assert((char)'abc' == 'c', ""); // CHECK: /multi-char/
_Static_assert(/*int*/'abc' == 0x616263, "");

_Static_assert(/*int*/'abcde' == 'bcde', ""); // CHECK: warning: ignoring extraneous characters in literal
_Static_assert(/*int*/'abcde' == 0x62636465, "");

_Static_assert(L'a' == 'a', "");
_Static_assert(L'ab' == 'b', ""); // CHECK: warning: ignoring extraneous characters in literal

_Static_assert('\xc' == 12, "");
_Static_assert('\3' == 3, "");

_Static_assert('\\' == 92, "");
_Static_assert('\"' == 34, "");
_Static_assert('\'' == 39, "");
_Static_assert('\?' == 63, "");
_Static_assert('\a' == 7, "");
_Static_assert('\b' == 8, "");
_Static_assert('\e' == 27, "");
_Static_assert('\f' == 12, "");
_Static_assert('\n' == 10, "");
_Static_assert('\r' == 13, "");
_Static_assert('\t' == 9, "");
_Static_assert('\v' == 11, "");

//#if '5692' != ((((5 * 256) + 6) * 256) + 9) * 256 + 2
//#  error wrong calc
//#endif

_Noreturn void abort(void);

static void assert(int x, int lno)
{
	if(!x){
		printf("failed on %d\n", lno);
	}
}
#define assert(x) assert((x), __LINE__)

int main()
{
	unsigned char x[] = "\037\213"; // check \213 sign-ext

	assert(x[0] == 037);
	assert(x[1] == 0213);

	return 0;
}
