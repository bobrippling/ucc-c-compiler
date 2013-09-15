abc(float a, float b)
{
	f(a == b); // nan = false
	f(a != b); // nan = true
	f(a > b); // nan = not checked
	f(a <= b); // nan = not checked
}
