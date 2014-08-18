f();

main()
{
	__asm("call %0" :: "i"(f)); // this is supposed to emit "call $f"
}
