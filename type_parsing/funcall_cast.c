#define IplImage void

cvGetSize(), cvCreateImage();

main()
{
	void *img;

#ifdef __TINYC__
	long long s = ((long long(*)(IplImage*))cvGetSize)(img);
	IplImage* gray = ((IplImage*(*)(long long, int, int))cvCreateImage)(s, 8, 1);
#else
#  ifdef __UCC__


	int s = ((int (*)(IplImage*))cvGetSize)(img);
	IplImage *gray = ((IplImage *(*)(int, int, int))cvCreateImage)(s, 8, 1);


#  else
	IplImage *gray = cvCreateImage( cvGetSize(img), 8, 1 );
#  endif
#endif
	return 0;
}
