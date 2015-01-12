#include "backend.h"
#include "isn.h"

int main()
{
	val *a = val_new_i(3);
	val *b = val_new_i(5);
	val *store = val_new_ptr_from_int(0);

	val_store(a, store);

	val *loaded = val_load(store);

	val *other_store = val_alloca(2);

	val_store(val_new_i(7), other_store);

	val *added = val_add(b,
			val_add(
				val_load(other_store),
				loaded));

	val *add_again = 
		val_add(
				val_add(
					val_load(store),
					val_load(other_store)),
				added);

	val *alloca_p = val_element(other_store, 1);

	val_add(val_load(alloca_p), val_new_i(33));

	isn_optimise();

	//isn_dump();
}
