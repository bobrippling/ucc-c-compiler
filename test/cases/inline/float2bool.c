// RUN: %ocheck 0 %s -fno-semantic-interposition
// RUN: %ucc -target x86_64-linux -S -o- %s -fno-semantic-interposition | grep 'movb $0, %%al'

__attribute((always_inline))
inline _Bool f(float f)
{
	return f;
}

main()
{
	return f(0);
}
