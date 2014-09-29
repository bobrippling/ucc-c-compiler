// RUN: %ocheck 0 %s

#define attr always_inline

int not_present = __has_attribute(attr);

main()
{
	if(not_present)
		abort();

	return 0;
}
