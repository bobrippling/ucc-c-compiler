void clean(void *);

void f(int a)
{
	switch(a){
		case 1: // CHECK: !/warn|error/
		{
			int x __attribute__((cleanup(clean))) = 5;

		case 2:
			;
		}
	}

	goto prot;
	{
		int x __attribute__((cleanup(clean))) = 5;
prot:
		;
	}
}
