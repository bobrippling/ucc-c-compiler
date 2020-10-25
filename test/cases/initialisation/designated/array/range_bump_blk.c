// RUN: %layout_check %s
// RUN: %ocheck 0 %s

int fs, gs;

extern void abort(void);

void f(void)
{
	fs++;
}

void g(void)
{
	gs++;
}

void (^blks[])() = {
	[0 ... 99] = ^{},
	[50] = ^{ f(); },
	[100] = ^{ g(); }
};

int main()
{
	for(int i = 0; i <= 100; i++){
		void (^b)() = blks[i];

		switch(i){
			case 50:
				if(fs)
					abort();
				b();
				if(fs != 1)
					abort();
				break;
			case 100:
				if(gs)
					abort();
				b();
				if(gs != 1)
					abort();
				break;
			default:
			{
				int f2 = fs, g2 = gs;
				b();
				if(f2 != fs)
					abort();
				if(g2 != gs)
					abort();
			}
		}
	}

	if(gs != 1)
		abort();
	if(fs != 1)
		abort();

	return 0;
}
