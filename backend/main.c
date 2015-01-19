#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "backend.h"
#include "isn.h"

static void eg(bool opt)
{
	val *a = val_new_i(3);
	val *b = val_new_i(5);
	val *store = val_new_ptr_from_int(0);

	// store = 3
	val_store(a, store);

	// loaded = 3
	val *loaded = val_load(store);

	val *other_store = val_alloca(2, 4);

	val_store(val_new_i(7), other_store);
	val_store(val_new_i(9), val_element(other_store, 1, 4));

	// other_store = { 7, 9 }

	val *added = val_add(b,
			val_add(
				val_load(other_store),
				loaded));

	// added = 5 + (7 + 3) = 15

	val *add_again =
		val_add(
				val_add(
					val_load(store),
					val_load(other_store)),
				added);

	// add_again = (3 + 7) + 15 = 25

	val *alloca_p = val_element(other_store, 1, 4);

	val_add(val_load(alloca_p), val_new_i(33));
	// 9 + 33 = 42

	isn_dump(opt);
}

static void usage(const char *arg0)
{
	fprintf(stderr, "Usage: %s [-O]\n", arg0);
	exit(1);
}

int main(int argc, char *argv[])
{
	bool opt = false;

	for(int i = 1; i < argc; i++){
		if(!strcmp(argv[i], "-O")){
			opt = true;
		}else{
			usage(*argv);
		}
	}

	eg(opt);

	return 0;
}
