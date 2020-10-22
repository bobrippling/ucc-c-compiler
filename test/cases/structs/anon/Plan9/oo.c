// RUN: %ocheck 0 %s -fplan9-extensions
void abort(void) __attribute__((noreturn));

typedef struct Animal Animal;

struct Animal
{
	int weight;
};

weights;

sum_weight(struct Animal *a)
{
	weights += a->weight;
}

main()
{
	struct Dog
	{
		int nlegs;
		Animal;
	};
	struct Cat
	{
		char *name;
		struct Animal;
	};

	struct Cat cat;
	struct Dog dog;

	cat.weight = 2;
	dog.Animal.weight = 5; // Plan 9 ext 2

	sum_weight(&cat); // Plan 9 ext 1
	sum_weight(&dog);

	if(weights != 7)
		abort();
	return 0;
}
