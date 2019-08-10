// RUN: %ucc -c %s

f();

main()
{
	if(int i = f());
}
