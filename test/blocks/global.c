// RUN: %ocheck 0 %s

int x;

int (^b)(int) = ^(int i){
	x = 10;
	return i + 1;
};

int main()
{
	if(b(2) != 3)
		abort();

	if(x != 10)
		abort();

	return 0;
}
