struct half_bytes
{
	int f_1;
	int bf_1 : 3;
	unsigned bf_2 : 7;
	int f_2;
};

pbf(struct half_bytes *p);
