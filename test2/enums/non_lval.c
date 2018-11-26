// RUN: %ocheck 0 %s -fno-const-fold

enum { A };

main()
{
	return A;
}
