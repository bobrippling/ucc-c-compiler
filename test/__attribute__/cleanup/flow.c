// RUN: %ocheck 0 %s

int n;

g(int *p)
{
	n++;
}

f(i)
{
	if(i){
		__attribute((cleanup(g))) x = 3;
	}else{
		__attribute((cleanup(g))) x = 7;
	}

	while(i){
		__attribute((cleanup(g))) x = 3;
		i--;
	}

	i = 5;
	do{
		__attribute((cleanup(g))) x = 3;
		i--;
	}while(i);

	for(i = 0; i < 5; i++){
		__attribute((cleanup(g))) x = 3;
	}

	for(int j = 0; j < 2; j++){
		__attribute((cleanup(g))) x = 3;
	}

	for(__attribute((cleanup(g))) int j = 0; j < 5; j++){
		__attribute((cleanup(g))) x = 3;
	}
}

_Noreturn void abort();

int *expected;
order(int *p)
{
	if(p != expected)
		abort();
	n++;
}

for_order()
{
	__attribute((cleanup(order))) int out = 0;

	for(__attribute((cleanup(order))) int in = 0; in < 2; in++){
		expected = &in;
	}

	expected = &out;
}

main()
{
	f(3);
	if(n != 22)
		abort();

	for_order();
	if(n != 24)
		abort();

	return 0;
}
