main()
{
	struct { int i; } a;
	&a; // most likely this is a problem is asm_sym, changing a mov to a lea and not qualifiying with ptr_depth_inc
}
