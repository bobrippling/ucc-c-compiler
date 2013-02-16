typedef struct Animal Animal;

struct Animal
{
	int weight;
};

show_weight(struct Animal *a)
{
	printf("weight %d\n", a->weight);
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
	dog.Animal.weight = 5; // needs typedef

	show_weight(&cat);
	show_weight(&dog);
}
