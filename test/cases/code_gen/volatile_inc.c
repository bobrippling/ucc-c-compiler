// RUN: %ocheck 0 %s

static volatile int i;

static void assert(_Bool b)
{
	if(!b){
		extern _Noreturn void abort(void);
		abort();
	}
}

int main()
{
	int r = i++;
	i++;
	assert(r == 0);
	assert(i == 2);
}
