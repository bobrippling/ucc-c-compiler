// RUN: %ucc -E -trigraphs %s

main()
{
	??a; // this would infinite loop previously
}
