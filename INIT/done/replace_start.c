// FIXME: overrides the first, and so breaks the rest which depend on it
// FIXME: test middle array-range overrides

#ifdef LOCAL
main()
{
#endif
const char x[] = {
	[0 ... 8] = 5,
	[0] = 1
};
#ifdef LOCAL
}
#endif
