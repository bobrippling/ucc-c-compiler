f(){ return 1; }
main()
{
	int sz = sizeof *( 0 ? 0 : (int(*)[f()]) 0 );
}
