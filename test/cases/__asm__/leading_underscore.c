// RUN:   %ucc -S -o %t %s -fno-leading-underscore -fno-pic -std=c89 -fno-common
// RUN: ! grep '^g_i' %t
// RUN:   grep '^no_leading' %t
// RUN:   grep '^g_j' %t
// RUN:   grep 'call.* implicit$' %t
// RUN:   grep 'call.* global$' %t
// RUN:   grep 'call.* global_renamed_hello$' %t
// RUN:   grep 'call.* local$' %t
// RUN:   grep 'call.* local_renamed_hello$' %t
// RUN:   grep '^main' %t
//
// RUN:   %ucc -S -o %t %s -fleading-underscore -fno-pic -std=c89 -fno-common
// RUN: ! grep '^g_i' %t
// RUN:   grep '^no_leading' %t
// RUN:   grep '^_g_j' %t
// RUN:   grep 'call.* _implicit$' %t
// RUN:   grep 'call.* _global$' %t
// RUN:   grep 'call.* global_renamed_hello$' %t
// RUN:   grep 'call.* _local$' %t
// RUN:   grep 'call.* local_renamed_hello$' %t
// RUN:   grep '^_main' %t

int g_i __asm("no_leading"), g_j;

extern global();
extern global_renamed() __asm("global_renamed_hello");

main()
{
	extern local();
	extern local_renamed() __asm("local_renamed_hello");

	implicit();

	global();
	global_renamed();

	local();
	local_renamed();

	g_i = g_j = 2;
	return g_i;
}
