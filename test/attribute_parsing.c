int __attribute((warn_unused)) i __attribute((unused)), j;

int __attribute((warn_unused)) __attribute__((type)) f() __attribute((f)) __attribute((unused))
{
}

main(void)
{
	return f();
}
