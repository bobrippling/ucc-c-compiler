// RUN: %ocheck 0 %s

#if !__has_attribute(format)
#error x
#endif

#if !__has_attribute(__format__)
#error x
#endif

#if __has_attribute(__format)
#error x
#endif

#if __has_attribute(format__)
#error x
#endif

#define attr always_inline

int not_present = __has_attribute(attr);

main()
{
	if(not_present)
		abort();

	return 0;
}
