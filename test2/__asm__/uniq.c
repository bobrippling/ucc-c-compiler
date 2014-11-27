// RUN: %ucc -S -o- %s | grep '^[ 	]*[0-9][0-9]*$' | %output_check -w 1 2 3
// RUN: %ucc -S -o- %s | grep -F 'no interp %='

main()
{
	__asm("%=" : );
	__asm("%=" : );
	__asm("%=" : );

	__asm("no interp %=");
}
