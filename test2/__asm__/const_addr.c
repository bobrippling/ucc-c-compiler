// RUN: %archgen %s 'x86_64,x86:call $f' -fno-leading-underscore

f();

main()
{
	__asm("call %0" :: "i"(f)); // this is supposed to emit "call $f"
}
