// RUN: %check -e %s

f(i)
{
	char buf[4] = (i ? "hi" : "yo"); // CHECK: error: char[4] must be initialised with an initialiser list
	//char ar[2] = (&(int){1});
}
