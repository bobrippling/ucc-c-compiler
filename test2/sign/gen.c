// RUN: %ucc -c %s
// RUN: %ucc -S -o- %s | grep 'seta '

f(unsigned i)
{
	return i > 5;
}
