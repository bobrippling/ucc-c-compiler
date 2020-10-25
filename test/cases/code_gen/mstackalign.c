// RUN: %ucc -target x86_64-linux -S -o- %s -mpreferred-stack-boundary=4 -mstackrealign | %stdoutcheck %s
//
//      STDOUT: /sub. \$16, %rsp/
// STDOUT-NEXT: /and. \$15, %rsp/

int g();

// we should realign this regardless
// - could be called from 4-byte stack aligned code
f()
{
	g(); // needed to force prologue/epilogue
}
