// RUN: %ucc -c -fleading-underscore %s
// RUN: ! %ucc -S -o- -fleading-underscore %s | grep '_no_leading'
// RUN: %ucc -S -o- -fleading-underscore %s | grep '_main'

int g_i asm("no_leading"), g_j;

extern global();
extern global_renamed() asm("global_renamed_HAI");

main()
{
	extern local();
	extern local_renamed() asm("local_renamed_HAI");

	implicit();

	global();
	global_renamed();

	local();
	local_renamed();

	g_i = g_j = 2;
	return g_i;
}
