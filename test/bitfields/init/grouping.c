// RUN: %layout_check %s

struct Padded
{
	int i : 2;
	int : 0;
	int j : 3;
	int : 4;
	int end : 13;
};

struct Padded pad = {
	1, 2, 3 // should initialise i, j and k, skipping unnamed fields
};

// -------

/* This results in using a short (decl_type_for_bitfield),
 * when we want to store the bitfield {c, d}.
 *
 * The bug is from fold_sue.c, when we decide whether to overflow into the next
 * bitfield group or not
 */
struct {
	int a;
	char b;
	int c : 14, d : 14;
	int e;
} two_ints = {
	0xaaaaaaaa,
	0xbb,
	0xccc,
	0xddd,
	0xeeeeeeee
};
