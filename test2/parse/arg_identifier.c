// RUN: %ucc -c %s

f(int i, int j, va_list);
// parsed as:
//f(int i, int j, int va_list);

main()
{
	f(1, 2, 3);
}
