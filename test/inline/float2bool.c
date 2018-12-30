// RUN: %ocheck 0 %s -fno-semantic-interposition
// RUN: %archgen %s 'x86_64,x86:/movb \$0, %%al/' -fno-semantic-interposition

__attribute((always_inline))
inline _Bool f(float f)
{
	return f;
}

main()
{
	return f(0);
}
