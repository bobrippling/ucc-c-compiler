// RUN: %ucc -fsyntax-only %s

main()
{
lbl
# 3 "tim.c" // parsing check
:

	return 0;
}
