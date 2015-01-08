#include "backend.h"

int main()
{
	val *a = val_new_i(3);
	val *b = val_new_i(5);
	val *store = val_new_ptr_from_int(0);

	val_store(store, a);

	val *loaded = val_load(store);

	val *added = val_add(b, loaded);

	val_show(added);
}
