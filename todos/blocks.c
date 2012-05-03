main()
{
	void (^f)() = ^{ printf("yo\n"); };

	f();
	__main_block_invoke_1(0);
}
