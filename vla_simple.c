int f(int x, int y[*]);
void g(int (*y)[*]);

int f(int x, int y[x])
{
	int buf[x];
	g(buf);
	return buf[2];
}
