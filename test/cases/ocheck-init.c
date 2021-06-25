// RUN: true

do{
#if !defined(__DARWIN__)
void randomise_stack(void) __attribute__((weak));
if(randomise_stack)
	randomise_stack();
#else
// weak support not present here
#endif
}while(0);
