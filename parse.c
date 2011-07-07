#include <stdio.h>

#include "parse.h"
#include "tokenise.h"

extern enum token curtok;



void parse()
{
	function **f;
	int i, n;

	n = 10;
	i = 0;
	f = umalloc(n * sizeof *f);

	do{
		f[i++] = function();

		if(i == n){
			n += 10;
			f = urealloc(f, n * sizeof *f);
		}
	}while(curtok != token_eof);
}
