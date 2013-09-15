fp_jmp(float a, float b)
{
	if(a == b) // nan = false
		eq();
	if(a != b) // nan = true
		ne();
	if(a > b) // nan = not checked
		gt();
	if(a <= b) // nan = not checked
		le();
}
