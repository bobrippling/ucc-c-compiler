typedef struct animal Animal;

struct animal
{
	void (*walk)(Animal *);
	void (*run)( Animal *);
};

Animal *animal_new()
{
	return malloc(sizeof *animal_new());
}

void run_crab(Animal *crab)
{
	printf("crabs can't run sideways\n");
}
void walk_crab(Animal *crab)
{
	printf("crabs walk sideways\n");
}

void run_dog(Animal *dog)
{
	printf("dogs run quickly\n");
}
void walk_dog(Animal *dog)
{
	printf("dogs walk\n");
}

#define ANIMAL_CONSTRUCTOR(fn, fwalk, frun) \
Animal *fn()                                \
{                                           \
	Animal *a = animal_new();                 \
	a->walk = fwalk;                          \
	a->run  = frun;                           \
	return a;                                 \
}


ANIMAL_CONSTRUCTOR(animal_new_dog,  walk_dog,  run_dog)
ANIMAL_CONSTRUCTOR(animal_new_crab, walk_crab, run_crab)

main()
{
	Animal *animals[2];

	animals[0] = animal_new_dog();
	animals[1] = animal_new_crab();

	for(int i = 0; i < 2; i++){
		animals[i]->walk(animals[i]); // parser problem
		animals[i]->run( animals[i]);
	}
}
