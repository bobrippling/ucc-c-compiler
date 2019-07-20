// RUN: %archgen %s 'x86:subl $16, %%esp' 'x86:andl $15, %%esp' 'x86_64:subq $16, %%rsp' 'x86_64:andq $15, %%rsp' -mpreferred-stack-boundary=4 -mstackrealign

int g();

// we should realign this regardless
// - could be called from 4-byte stack aligned code
f()
{
	g(); // needed to force prologue/epilogue
}
