// RUN: %ucc -target x86_64-linux -S -o %t %s
// RUN: %stdoutcheck %s <%t
//
// RUN: %ucc -target x86_64-linux -S -o %t %s -mstackrealign
// RUN: %stdoutcheck --prefix=noredzone %s <%t
//
// RUN: %ucc -target x86_64-linux -S -o %t %s -mno-red-zone
// RUN: %stdoutcheck --prefix=noredzone %s <%t

int f1(void)
{
	char buf[124]; // big stack, can't red-zone
	for(int i = 0; i < sizeof buf; i++)
		buf[i] = 0;
	return buf[0];

	// STDOUT:     /^f1:/
	// STDOUT:     /sub.*rsp/
	// STDOUT:     leave
	// STDOUT:     ret

	// STDOUT-noredzone: /^f1:/
	// STDOUT-noredzone: /sub.*rsp/
	// STDOUT-noredzone: leave
	// STDOUT-noredzone: ret
}

int f2(void)
{
	char buf[120]; // small stack, can red-zone
	for(int i = 0; i < sizeof buf; i++)
		buf[i] = 0;
	return buf[0];

	// STDOUT:     /^f2:/
	// STDOUT-NOT: sub.*sp
	// STDOUT:     ret

	// STDOUT-noredzone: /^f2:/
	// STDOUT-noredzone: /sub.*rsp/
	// STDOUT-noredzone: leave
	// STDOUT-noredzone: ret
}

int g(int n)
{
	int vla[n]; // stack manipulation, can't red-zone
	vla[3] = 1;
	return 5;

	// STDOUT:     /^g:/
	// STDOUT:     /sub.*rsp/
	// STDOUT:     leave
	// STDOUT:     ret

	// STDOUT-noredzone: /^g:/
	// STDOUT-noredzone: /sub.*rsp/
	// STDOUT-noredzone: leave
	// STDOUT-noredzone: ret
}

int h(void)
{
	f1(); // call, can't red-zone
	return 3;

	// STDOUT:     /^h:/
	// no sub of stack pointer - no stack used
	// STDOUT:     leave
	// STDOUT:     ret

	// STDOUT-noredzone: /^h:/
	// no sub of stack pointer (for -mno-red-zone) - no stack used
	// STDOUT-noredzone: leave
	// STDOUT-noredzone: ret
}
