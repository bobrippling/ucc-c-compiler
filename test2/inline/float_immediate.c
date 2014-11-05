// RUN: %ocheck 1 %s

__attribute((always_inline))
inline or(float a)
{
	return a;
}

main()
{
	return or(1);
}
