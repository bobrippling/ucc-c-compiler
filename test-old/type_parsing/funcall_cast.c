#define IplImage void

cvGetSize()
{
	printf("size()\n");
}

cvCreateImage()
{
	printf("create()\n");
}

main()
{
	void *img;

	int s = ((int (*)(IplImage *))cvGetSize)(img);
	IplImage *gray = ((IplImage *(*)(int, int, int))cvCreateImage)(s, 8, 1);

	return 0;
}
