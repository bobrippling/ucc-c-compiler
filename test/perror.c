main()
{
	extern errno = 2;
	perror(0);
	perror("open()");
}
