// RUN: %ucc -Wall -fsyntax-only %s 2>&1|grep -F "f();  "

main()
{
	f();
}
