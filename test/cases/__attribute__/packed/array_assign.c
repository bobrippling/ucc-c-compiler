// RUN: %ocheck 0 %s

struct __attribute((packed)) A
{
	char c; int i;
};

void assert(_Bool b)
{
	if(!b){
		void abort(void);
		abort();
	}
}

main()
{
	char bytes[1 + sizeof(int)] = {
		1, 2, 0, 0, 0
	};

	struct A a = *(struct A *)bytes;

	assert(a.c == 1);
	assert(a.i == 2);

	return 0;
}
