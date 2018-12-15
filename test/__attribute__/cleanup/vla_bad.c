// RUN: %check -e %s

chk_vla(short *p);

f(int n)
{
	short vla[n] __attribute((cleanup(chk_vla))); // CHECK: error: type 'short (*)[vla]' passed - cleanup needs 'short *'
}
