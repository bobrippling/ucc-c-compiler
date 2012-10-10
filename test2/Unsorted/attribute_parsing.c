int __attribute((warn_unused)) i __attribute((unused)), j;

int __attribute((warn_unused)) __attribute__((type)) f() __attribute((f)) __attribute((unused))
{
	return 3;
}

main(void)
{
	__attribute__(())a = 2;
	return f() + a;
}
