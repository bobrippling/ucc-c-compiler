// RUN: %ocheck 2 %s
i;

f(){ return 1; }
p(){ i++; }

main()
{
#include "../ocheck-init.c"
	for(int j; 0;); // valid

	for(int i = f(); i < 3; i++)
		p();

	return i;
}
