// RUN: %ucc -o %t %s
// RUN: %t

int cnt;

main()
{
	int y[] = { 1, 2 };

	for(int i = 0; i < 2; i++) cnt++;
	for(int x[] = { 1, 2, 3 }; 0;) cnt++;

	return y[1] == 2 && cnt == 2 ? 0 : 1;
}
