// RUN: %ucc -emit=print %s | grep -F "a 'typeof(expr: val) (aka 'int')'"

main()
{
	__typeof(5) a;
}
