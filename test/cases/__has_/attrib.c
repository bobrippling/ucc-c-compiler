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

/* test aliases */
#if !__has_attribute(warn_unused_result)
#error x
#endif

#define attr always_inline

int not_present = __has_attribute(attr);

main()
{
#include "../ocheck-init.c"
	if(not_present){
		_Noreturn void abort();
		abort();
	}

	return 0;
}
