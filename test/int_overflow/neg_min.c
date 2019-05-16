// RUN: %ocheck trap %s -ftrapv

#define INT64_MAX (-1ull / 2)
#define INT64_MIN (-INT64_MAX - 1)

typedef long long int64_t;

f(int64_t x)
{
	(void)x;
}

int main()
{
	int64_t min = INT64_MIN;

	f(-min);

	return 0;
}
