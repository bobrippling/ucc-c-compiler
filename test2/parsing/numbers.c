// RUN: %ocheck 31 %s
f(){return 3;}
main()
{
	int d = 0;
	int x = 0x5;
	int o = 05;
	int b = 0b10101;

	switch(f()){
		case 0 ... 5:
			return d + x + o + b; // 31
	}

	return 99;
}
