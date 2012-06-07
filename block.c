f(int x)
{
	int (^b)();

	b = ^{
		printf("hi\n");
		if(x)
			return 3;
	};

	return b();
}

/*
int (^f)(int) = ^(int a){
	return a + 5;
};

^{printf("yo\n");}();
*/
