// RUN: %ucc -Xprint %s | grep -F "typeof(expr: val) (aka 'int') a"

main()
{
	__typeof(5) a;
}
