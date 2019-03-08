// RUN: %ocheck 2 %s

int main()
{
	int x = 10;

	// ensure we flush the subtractions on the registers before the division
	return (x-2)/ (x-7);
}
