// RUN: %ucc -c %s; [ $? -ne 0 ]
f()
{
	struct B a[] = { 1 };
}
