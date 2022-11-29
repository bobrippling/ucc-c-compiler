// RUN: %ucc -c -fleading-underscore %s
// RUN: %ucc -S -fleading-underscore %s -o %t
// RUN: ! grep _no_leading %t
// RUN:   grep _main %t

int g_i __asm("no_leading"), g_j;

extern global();
extern global_renamed() __asm("global_renamed_HAI");

main()
{
	extern local();
	extern local_renamed() __asm("local_renamed_HAI");

	implicit();

	global();
	global_renamed();

	local();
	local_renamed();

	g_i = g_j = 2;
	return g_i;
}
