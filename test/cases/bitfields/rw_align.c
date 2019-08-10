// RUN: %ocheck 0 %s
// all accesses should be 4-byte aligned (int)
// RUN: %ucc -fno-inline-functions -S -o- %s | awk '/main/,/^}/' | grep '[0-9](%%rbp' | grep -v '4(%%rbp'; [ $? -eq 1 ]

struct bits
{
	unsigned first  : 1;
	unsigned second : 1;
};

chk(struct bits *p)
{
	// check the bottom two bits
	char v = 0x3 & *(char *)p;
	if(v != 3){
		_Noreturn void abort();
		abort();
	}
}

main()
{
	struct bits two_bit;

	two_bit.first = 1;
	two_bit.second = 1;

	chk(&two_bit);
	return 0;
}
