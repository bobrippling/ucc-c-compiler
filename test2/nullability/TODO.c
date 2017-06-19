int f(int *_Nullable p)
{
	return *p;
}

int g(int *_Nonnull p)
{
	return *p;
}

int h(int *_Nullable p)
{
	if(p){
		return *p;
	}
	return 3;
}

int main()
{
	f(&(int){2});
	f((void *)0);
	g(&(int){2});
	g((void *)0);
	h(&(int){2});
	h((void *)0);

	int *          a = (int *_Nullable)0;
	int *          b = (int *_Nonnull )0;
	int *_Nullable c = (int *_Nullable)0;
	int *_Nonnull  d = (int *_Nonnull )0; // warn
	int *_Nullable e = (int *         )0;
	int *_Nonnull  f = (int *         )0; // warn
}
