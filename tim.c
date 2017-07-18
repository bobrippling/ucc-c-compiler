int f(int *_Nullable p)
{
	if(p){
		return *p;
	}
	return 2;
}
