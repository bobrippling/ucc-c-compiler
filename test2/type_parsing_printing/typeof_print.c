// RUN: %ucc -emit=print %s | grep -F "a 'typeof(expr: value) (aka 'int')'"

main()
{
	__typeof(5) a;
}
