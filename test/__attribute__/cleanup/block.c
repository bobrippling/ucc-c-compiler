// RUN: %ocheck 0 %s

#define JOIN_(a, b) a##b
#define JOIN(a, b) JOIN_(a, b)

#define defer void (^JOIN(defer_, __LINE__))(void) __attribute__((cleanup(clean_block))) = ^

_Noreturn
void abort(void);

static void clean_block(void (^*blk)(void)) {
	(*blk)();
}

static int i;
static int j;

static void f(void)
{
	defer {
		i = 3;
	};
	defer {
		i = 2;
		j = 1;
	};
}

int main()
{
	f();
	if(i != 3)
		abort();
	if(j != 1)
		abort();
	return 0;
}
