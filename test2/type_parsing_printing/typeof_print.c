// RUN: %ucc -Xprint %s | grep -F "typeof(int) a"
// RUN: %ucc -Xprint %s | grep -F "typeof(expr: val) (aka 'int') b"

main()
{
	typeof(int) a; // typeof(int) a
	typeof(5)   b; // typeof(expr: val) aka 'int' b
}
