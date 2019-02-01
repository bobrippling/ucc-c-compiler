// RUN: %ocheck 0 %s

#define ASM_LABEL(name)  \
__asm(#name ":"); \
extern char name[] __asm(#name)

void *call_location(void)
{
	return __builtin_extract_return_addr(__builtin_return_address(0));
}

extern char after_main[];

ASM_LABEL(before_main);
int main()
{
	void *p = call_location();

	if(before_main < p && p < after_main)
		return 0;

	return 1;
}
ASM_LABEL(after_main);
