/* the function call causes the other operand to be stack-saved */

int_sum(int *p)
{
	return *p + int_sum(0);
}
