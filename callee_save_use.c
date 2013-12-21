f(int stream, int ptr, int size)
{
	write(fileno(stream), ptr, size);
}
