typedef unsigned size_t;

void duff_memcpy(char *to, char *from, size_t count)
{
	size_t n = (count + 7) / 8;

	switch(count % 8){
		case 0: do{ *to++ = *from++;
		case 7:     *to++ = *from++;
		case 6:     *to++ = *from++;
		case 5:     *to++ = *from++;
		case 4:     *to++ = *from++;
		case 3:     *to++ = *from++;
		case 2:     *to++ = *from++;
		case 1:     *to++ = *from++;
		}while(--n>0);
	}
}
