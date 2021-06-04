// RUN: true

do{
void randomise_stack(void) __attribute__((weak));
if(randomise_stack)
	randomise_stack();
}while(0);
