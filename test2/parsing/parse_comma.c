// RUN: %ocheck 2 %s
void f_void(){}
int f_int(){return 2;}
main()
{
	int i;

	i = f_int(), f_void();

	return i;
}
