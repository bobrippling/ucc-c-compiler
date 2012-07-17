main()
{
#if 0
	abort();
#else
	printf("hi\n");
#endif

#if 1
	printf("hi\n");
#else
	abort();
#endif
}
