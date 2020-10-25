// RUN: %ocheck 2 %s
i;

f(){ return 1; }
p(){ i++; }

main()
{
	for(int j; 0;); // valid

	for(int i = f(); i < 3; i++)
		p();

	return i;
}
