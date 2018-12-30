// RUN: %ocheck 0 %s
struct Basic
{
	int x : 4, y : 4;
} bas = { 1, 2 };

main()
{
	if(bas.x != 1)
		return 1;
	if(bas.y != 2)
		return 1;
	if(*(char *)&bas != 33)
		return 1;
	return 0;
}
