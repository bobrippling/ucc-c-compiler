// RUN: %ucc -c %s
// RUN: %ucc -S -o- %s | grep -v 59
main()
{
	struct A
	{
		int i;
		int : 4;
	} a = {
		1,
		59 /* excess */
	};
}
