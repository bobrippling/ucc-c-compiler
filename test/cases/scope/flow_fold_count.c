// RUN: test $(%ucc %s 2>&1 | grep -c "shadows local") -eq 4

g()
{
}

f(int n)
{
	for(int n = n; n > 0; n--)
		g();

	if(int n = n)
		n;

	while(int n = n)
		n;

	switch(int n = n)
		n;
}

main()
{
	f(0);
}
