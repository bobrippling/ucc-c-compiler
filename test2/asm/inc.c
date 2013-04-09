// RUN: [ `%ucc %s -S -o- | grep -c call` -eq 1 ]
main()
{
	int i;
	i++;
	f();
}
